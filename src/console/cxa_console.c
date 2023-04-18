/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_console.h"


// ******** includes ********
#include <cxa_array.h>
#include <cxa_assert.h>
#include <cxa_delay.h>
#include <cxa_fixedFifo.h>
#include <cxa_numberUtils.h>
#include <cxa_runLoop.h>
#include <cxa_stringUtils.h>
#include <string.h>

#if defined(__XC) || defined(__TI_COMPILER_VERSION__)
#include <strtok.h>
#endif


#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define HEADER_NUM_COLS					40
#define COMMAND_PROMPT					" > "
#define ESCAPE							"\x1b"
#define CONSOLE_RESPONSE_TIMEOUT_MS		2000

#define ESCAPE_SEQUENCE_TIMEOUT_MS		500


// ******** local type definitions ********
typedef struct
{
	char command[CXA_CONSOLE_MAX_COMMAND_LEN_BYTES+1];
	char description[CXA_CONSOLE_MAX_DESCRIPTION_LEN_BYTES+1];

	cxa_console_command_cb_t cb;

	cxa_console_argDescriptor_t argDescs[CXA_CONSOLE_MAXNUM_ARGS];
	size_t numArgs;

	void* userVar;
}commandEntry_t;


// ******** local function prototypes ********
static void cb_onRunLoopStart(void* userVarIn);
static void cb_onRunLoopUpdate(void* userVarIn);
static void printBootHeader(const char* deviceNameIn);
static void printCommandLine(bool addPreLineEndingIn);
static void printBlockLine(const char* textIn, size_t maxNumCols);
static void clearScreenReturnHome(void);
static void clearBuffer(void);

static void command_clear(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void command_help(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_ioStream_t* ioStream = NULL;

static const char* deviceName = NULL;

static cxa_array_t commandBuffer;
static char commandBuffer_raw[CXA_CONSOLE_COMMAND_BUFFER_LEN_BYTES];

static cxa_fixedFifo_t commandBufferHistory;
static char commandBufferHistory_raw[CXA_CONSOLE_MAXLEN_COMMAND_HISTORY*CXA_CONSOLE_COMMAND_BUFFER_LEN_BYTES];
static ssize_t cbhScrollbackIndex = -1;

static cxa_array_t commandEntries;
static commandEntry_t commandEntries_raw[CXA_CONSOLE_MAXNUM_COMMANDS+2];
// add one for 'clear' and 'help' command

static bool isExecutingCommand = false;
static bool isPaused = false;


// ******** global function implementations ********
void cxa_console_init(const char* deviceNameIn, cxa_ioStream_t *const ioStreamIn, int threadIdIn)
{
	cxa_assert(ioStreamIn);

	// save our references
	ioStream = ioStreamIn;
	deviceName = deviceNameIn;

	// setup our arrays
	cxa_array_initStd(&commandBuffer, commandBuffer_raw);
	cxa_array_initStd(&commandEntries, commandEntries_raw);
	cxa_fixedFifo_init(&commandBufferHistory, CXA_FF_ON_FULL_DEQUEUE, sizeof(commandBuffer_raw), commandBufferHistory_raw, sizeof(commandBufferHistory_raw));

	// add our basic console commands
	cxa_console_addCommand("clear", "clears the console", NULL, 0, command_clear, NULL);
	cxa_console_addCommand("help", "prints available commands", NULL, 0, command_help, NULL);

	// register for our runLoop
	cxa_runLoop_addEntry(threadIdIn, cb_onRunLoopStart, cb_onRunLoopUpdate, NULL);
}


void cxa_console_addCommand(const char* commandIn, const char* descriptionIn,
							cxa_console_argDescriptor_t *const argDescsIn, size_t numArgsIn,
							cxa_console_command_cb_t cbIn, void* userVarIn)
{
	cxa_assert(commandIn);
	cxa_assert(strlen(commandIn) <= CXA_CONSOLE_MAX_COMMAND_LEN_BYTES);
	cxa_assert(cbIn);
	if( numArgsIn > 0 ) cxa_assert(argDescsIn);

	commandEntry_t newEntry = {
			.cb = cbIn,
			.userVar = userVarIn,
			.numArgs = numArgsIn
	};
	cxa_stringUtils_copy(newEntry.command, commandIn, sizeof(newEntry.command));
	cxa_stringUtils_copy(newEntry.description, (descriptionIn != NULL) ? descriptionIn : "<none>", sizeof(newEntry.description));
	if( numArgsIn > 0 ) memcpy(newEntry.argDescs, argDescsIn, sizeof(*argDescsIn) *numArgsIn);

	cxa_assert(cxa_array_append(&commandEntries, &newEntry));
}


void cxa_console_printErrorToIoStream(cxa_ioStream_t *const ioStreamIn, const char *const errorIn)
{
	cxa_assert(ioStreamIn);
	cxa_assert(errorIn);

	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
	cxa_ioStream_writeString(ioStream, "!! ");
	cxa_ioStream_writeLine(ioStream, errorIn);
}


bool cxa_console_isPaused(void)
{
    return isPaused;
}


void cxa_console_pause(void)
{
    isPaused = true;
}


void cxa_console_resume(void)
{
    isPaused = false;

    clearBuffer();
    printCommandLine(true);
}


bool cxa_console_isExecutingCommand(void)
{
	return isExecutingCommand;
}


void cxa_console_prelog(void)
{
	if( ioStream == NULL ) return;

	if( !isExecutingCommand )
	{
		cxa_ioStream_writeString(ioStream, ESCAPE "[2K");
		cxa_ioStream_writeByte(ioStream, '\r');
		cxa_ioStream_writeString(ioStream, ESCAPE "[1A");
	}
}


void cxa_console_postlog(void)
{
	if( ioStream == NULL ) return;

	if( !isExecutingCommand ) printCommandLine(true);
}


// ******** local function implementations ********
static void cb_onRunLoopStart(void* userVarIn)
{
	printBootHeader(deviceName);
	printCommandLine(true);
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
    // don't do anything if we're paused
    if( isPaused ) return;

	uint8_t rxByte;
	size_t numBytesInBuffer = cxa_array_getSize_elems(&commandBuffer);
	if( cxa_ioStream_readByte(ioStream, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
	{
		/// first byte is escape...don't let them get into the buffer and be repeated back

		// handle escape sequences
		if( rxByte == ESCAPE[0] )
		{
			cxa_timeDiff_t td_escape;
			cxa_timeDiff_init(&td_escape);

			uint8_t escapeByte_0, escapeByte_1;
			cxa_timeDiff_setStartTime_now(&td_escape);
			while( cxa_ioStream_readByte(ioStream, &escapeByte_0) != CXA_IOSTREAM_READSTAT_GOTDATA ) { if( cxa_timeDiff_isElapsed_ms(&td_escape, ESCAPE_SEQUENCE_TIMEOUT_MS) ) return; }
			cxa_timeDiff_setStartTime_now(&td_escape);
			while( cxa_ioStream_readByte(ioStream, &escapeByte_1) != CXA_IOSTREAM_READSTAT_GOTDATA ) { if( cxa_timeDiff_isElapsed_ms(&td_escape, ESCAPE_SEQUENCE_TIMEOUT_MS) ) return; }
			// if we made it here, we have a proper escape sequence

			size_t cbhScrollbackSize_elems = cxa_fixedFifo_getSize_elems(&commandBufferHistory);
			if( (escapeByte_0 == 0x5B) && (escapeByte_1 == 0x41) )
			{
				// up arrow
				if( cbhScrollbackSize_elems == 0 ) return;
				if( cbhScrollbackIndex == -1 ) cbhScrollbackIndex = 0;										// account for hitting our lower limit
				if( cbhScrollbackIndex < cbhScrollbackSize_elems )
				{
					cxa_fixedFifo_peekAtIndex(&commandBufferHistory, cbhScrollbackIndex++, commandBuffer_raw);
					cxa_array_init_inPlace(&commandBuffer, sizeof(*commandBuffer_raw), strlen(commandBuffer_raw), commandBuffer_raw, sizeof(commandBuffer_raw));
					// clear the line, reset the cursor to the beginning of the line
					cxa_ioStream_writeString(ioStream, ESCAPE "[2K\r");
					printCommandLine(false);
				}
			}
			else if( (escapeByte_0 == 0x5B) && (escapeByte_1 == 0x42) )
			{
				// down arrow
				if( cbhScrollbackIndex >= 0 )
				{
					if( cbhScrollbackIndex == cbhScrollbackSize_elems ) cbhScrollbackIndex -= 1;			// account for hitting our upper limit

					cxa_fixedFifo_peekAtIndex(&commandBufferHistory, cbhScrollbackIndex--, commandBuffer_raw);
					cxa_array_init_inPlace(&commandBuffer, sizeof(*commandBuffer_raw), strlen(commandBuffer_raw), commandBuffer_raw, sizeof(commandBuffer_raw));
					// clear the line, reset the cursor to the beginning of the line
					cxa_ioStream_writeString(ioStream, ESCAPE "[2K\r");
					printCommandLine(false);
				}
			}
		}
		else if( (rxByte == '\r') || (rxByte == '\n') )
		{
			// handle carriage returns / line feeds
			// end of a command...make sure it's not empty
			if( cxa_array_isEmpty(&commandBuffer) ) return;

			// split our command line by spaces...this _will_ mangle our commandBuffer
			char* tokSavePtr;
			char* cmd = strtok_r(cxa_array_get(&commandBuffer, 0), " ", &tokSavePtr);

			// look for our command
			bool foundCommand = false;
			cxa_array_iterate(&commandEntries, currEntry, commandEntry_t)
			{
				if( currEntry == NULL) continue;

				if( (cmd != NULL) && strcmp(cmd, currEntry->command) == 0 )
				{
					foundCommand = true;

					// we have a matching command...get ready to parse the arguments
					// (if we have them)
					cxa_array_t args;
					cxa_stringUtils_parseResult_t args_raw[CXA_CONSOLE_MAXNUM_ARGS];
					if( currEntry->numArgs > 0 ) cxa_array_initStd(&args, args_raw);

					// parse the arguments
					char* currParam_str;
					bool argParseError = false;
					for( size_t i = 0; i < currEntry->numArgs; i++ )
					{
						cxa_console_argDescriptor_t* currArgDesc = &currEntry->argDescs[i];

						// pull our argument string
						currParam_str = strtok_r(NULL, " ", &tokSavePtr);
						if( currParam_str == NULL )
						{
							cxa_console_printErrorToIoStream(ioStream, "Too few arguments");
							argParseError = true;
							break;
						}

						// parse it into a known data type and store to our array
						cxa_stringUtils_parseResult_t* parseVal = cxa_array_append_empty(&args);
						if( !cxa_stringUtils_parseString(currParam_str, parseVal) ||
							(parseVal->dataType != currArgDesc->dataType) )
						{
							cxa_console_printErrorToIoStream(ioStream, "Incorrect argument type(s)");
							argParseError = true;
							break;
						}

					}
					// make sure we break out of the outer loop as wel
					if( argParseError ) break;

					// make sure we don't have any extra arguments
					if( strtok_r(NULL, " ", &tokSavePtr) != NULL )
					{
						cxa_console_printErrorToIoStream(ioStream, "Too many arguments");
						break;
					}

					// if we made it here, we parsed our arguments correctly...
					// save to our command history and reset our scrollback index
					cxa_fixedFifo_queue(&commandBufferHistory, commandBuffer_raw);
					cbhScrollbackIndex = -1;

					// give it two new lines, then call the callback
					isExecutingCommand = true;
					cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);

					if( currEntry->cb != NULL ) currEntry->cb((currEntry->numArgs > 0) ? &args : NULL, ioStream, currEntry->userVar);
					cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
					break;
				}
			}
			if( !foundCommand )
			{
				cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
				cxa_console_printErrorToIoStream(ioStream, "Unknown command");
			}

			clearBuffer();
			printCommandLine(true);
		}
		else if( (rxByte == 0x08) || (rxByte == 0x7F) )
		{
			// handle backspaces / deletes
			if( numBytesInBuffer > 0 )
			{
				cxa_array_remove_atIndex(&commandBuffer, numBytesInBuffer - 1);
				// backspace moves the cursor back but doesn't clear the character...clear it, then go back again
				cxa_ioStream_writeByte(ioStream, rxByte);
				cxa_ioStream_writeByte(ioStream, ' ');
				cxa_ioStream_writeByte(ioStream, rxByte);
			}
		}
		else
		{
			// if we made it here, we're still in the middle of a command
			if( !cxa_array_append(&commandBuffer, &rxByte) )
			{
				// commandBuffer is full
				clearBuffer();
				cxa_console_printErrorToIoStream(ioStream, "Command too long for buffer");
				printCommandLine(true);
			}
			else
			{
				cxa_ioStream_writeByte(ioStream, rxByte);
			}
		}
	}
}


static void printBootHeader(const char* deviceNameIn)
{
	clearScreenReturnHome();

	for( int i = 0; i < HEADER_NUM_COLS; i++ ) cxa_ioStream_writeByte(ioStream, '*');
	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);

	if( deviceNameIn != NULL ) printBlockLine(deviceNameIn, HEADER_NUM_COLS);
	printBlockLine("Type 'help' for list of commands", HEADER_NUM_COLS);

	for( int i = 0; i < HEADER_NUM_COLS; i++ ) cxa_ioStream_writeByte(ioStream, '*');
	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
}


static void printCommandLine(bool addPreLineEndingIn)
{
	if( addPreLineEndingIn ) cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
	cxa_ioStream_writeString(ioStream, " > ");
	cxa_array_iterate(&commandBuffer, currCharPtr, char)
	{
		if( currCharPtr == NULL ) continue;

		cxa_ioStream_writeByte(ioStream, *currCharPtr);
	}
	isExecutingCommand = false;
}


static void printBlockLine(const char* textIn, size_t maxNumCols)
{
	cxa_assert(textIn);

	cxa_ioStream_writeByte(ioStream, '*');
	cxa_ioStream_writeByte(ioStream, ' ');

	// make sure to account for leading and trailing ' *'
	int totalPadding_spaces = HEADER_NUM_COLS - strlen(textIn) - 4;
	if( totalPadding_spaces < 0 )
	{
		totalPadding_spaces = 0;
	}
	int leftPadding_spaces = totalPadding_spaces / 2;
	int rightPadding_spaces = totalPadding_spaces  - leftPadding_spaces;

	for( int i = 0; i < leftPadding_spaces; i++ ) cxa_ioStream_writeByte(ioStream, ' ');
	cxa_ioStream_writeBytes(ioStream, (void*)textIn, CXA_MIN(strlen(textIn), HEADER_NUM_COLS - 4));
	for( int i = 0; i < rightPadding_spaces; i++ ) cxa_ioStream_writeByte(ioStream, ' ');

	cxa_ioStream_writeByte(ioStream, ' ');
	cxa_ioStream_writeByte(ioStream, '*');
	cxa_ioStream_writeString(ioStream, CXA_LINE_ENDING);
}


static void clearScreenReturnHome(void)
{
	// clear screen, then set cursor to home
	cxa_ioStream_writeString(ioStream, ESCAPE "[2J");
	cxa_ioStream_writeString(ioStream, ESCAPE "[H");
}


static void clearBuffer(void)
{
	cxa_array_clear(&commandBuffer);
	memset(cxa_array_get_noBoundsCheck(&commandBuffer, 0), 0, cxa_array_getMaxSize_elems(&commandBuffer));
}


static void command_clear(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	clearScreenReturnHome();
}


static void command_help(cxa_array_t *const argsIn, cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	cxa_ioStream_writeLine(ioStream, "Available commands:");
	cxa_array_iterate(&commandEntries, currEntry, commandEntry_t)
	{
		if( currEntry == NULL ) continue;

		// first the command and description
		for( int i = 0; i < 3; i++ ) cxa_ioStream_writeString(ioStream, " ");
		cxa_ioStream_writeString(ioStream, (char*)currEntry->command);
		for( int i = 0; i < CXA_CONSOLE_MAX_COMMAND_LEN_BYTES - strlen(currEntry->command); i++ ) cxa_ioStream_writeString(ioStream, " ");
		for( int i = 0; i < 3; i++ ) cxa_ioStream_writeString(ioStream, " ");
		cxa_ioStream_writeLine(ioStream, (char*)currEntry->description);

		// now the parameters (if applicable)
		if( currEntry->numArgs > 0 )
		{
			// now the parameters header
//			for( int i = 0; i < CXA_CONSOLE_MAX_COMMAND_LEN_BYTES+6; i++ ) cxa_ioStream_writeString(ioStream, " ");
//			cxa_ioStream_writeLine(ioStream, "Parameters:");

			// now each parameter
			for( size_t i = 0; i < currEntry->numArgs; i++ )
			{
				cxa_console_argDescriptor_t* currParam = &currEntry->argDescs[i];

				for( int i = 0; i < CXA_CONSOLE_MAX_COMMAND_LEN_BYTES+6; i++ ) cxa_ioStream_writeString(ioStream, " ");
				const char* paramType = cxa_stringUtils_getStringForDataType(currParam->dataType);
				cxa_ioStream_writeFormattedString(ioStream, "<%s>", paramType);
				for( int i = 0; i < 11 - strlen(paramType); i++ ) cxa_ioStream_writeString(ioStream, " ");
				cxa_ioStream_writeLine(ioStream, (char*)currParam->description);
			}
		}
	}
}
