/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_commandLineParser.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_stringUtils.h>
#include <cxa_config.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void helpCallback(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void assembleUsageString(cxa_commandLineParser_optionEntry_t *const optEntryIn, size_t maxStrLen_bytesIn, char *const stringOut);
static cxa_commandLineParser_optionEntry_t* findOptionByLongOpt(cxa_commandLineParser_t *const clpIn, char *passedOpt);
static cxa_commandLineParser_optionEntry_t* findOptionByShortOpt(cxa_commandLineParser_t *const clpIn, char *passedOpt);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_commandLineParser_init(cxa_commandLineParser_t *const clpIn, char *const progNameIn, char *const descriptionIn)
{
	cxa_assert(clpIn);
	cxa_assert(progNameIn);

	// save our references
	clpIn->progName = progNameIn;
	clpIn->description = descriptionIn;

	// setup our options array
	cxa_array_initStd(&clpIn->options, clpIn->options_raw);

	// add our help options
	cxa_commandLineParser_addOption_noArg(clpIn, "h", "help", "prints command help and exits", false, helpCallback, NULL);
	cxa_commandLineParser_addOption_noArg(clpIn, "?", NULL, "prints command help and exits", false, helpCallback, NULL);
}


bool cxa_commandLineParser_addOption_noArg(cxa_commandLineParser_t *const clpIn,
										   char *const shortOptIn, char *const longOptIn, char *const descIn, bool isRequiredIn,
										   cxa_commandLineParser_cb_noArg_t cbIn, void *const userVarIn)
{
	cxa_assert(clpIn);
	cxa_assert(shortOptIn || longOptIn);

	// create our option entry
	cxa_commandLineParser_optionEntry_t newEntry =
			{
					.shortOpt=shortOptIn, .longOpt=longOptIn, .description=descIn, .isRequired=isRequiredIn,
					.hasArgument=false,
					.cb_noArg=cbIn, .cb_arg=NULL, .userVar=userVarIn
			};
	return cxa_array_append(&clpIn->options, &newEntry);
}


bool cxa_commandLineParser_addOption_arg(cxa_commandLineParser_t *const clpIn,
										 char *const shortOptIn, char *const longOptIn, char *const descIn, bool isRequiredIn,
										 bool isArgumentRequiredIn, cxa_stringUtils_dataType_t expectedArgumentTypeIn,
										 cxa_commandLineParser_cb_arg_t cbIn, void *const userVarIn)
{
	cxa_assert(clpIn);
	cxa_assert(shortOptIn || longOptIn);

	// create our option entry
	cxa_commandLineParser_optionEntry_t newEntry =
				{
						.shortOpt=shortOptIn, .longOpt=longOptIn, .description=descIn, .isRequired=isRequiredIn,
						.hasArgument=true, .isArgumentRequired=isArgumentRequiredIn, .expectedArgType=expectedArgumentTypeIn,
						.cb_noArg=NULL, .cb_arg=cbIn, .userVar=userVarIn
				};
	return cxa_array_append(&clpIn->options, &newEntry);
}


void cxa_commandLineParser_printUsage(cxa_commandLineParser_t *const clpIn)
{
	cxa_assert(clpIn);

	printf("Usage: %s <options>" CXA_LINE_ENDING, clpIn->progName);
	if( clpIn->description != NULL ) printf("%s" CXA_LINE_ENDING, (clpIn->description != NULL) ? clpIn->description : "<no description>");
	printf(CXA_LINE_ENDING);

	char usageString[30];
	cxa_array_iterate( &clpIn->options, currOptEntry, cxa_commandLineParser_optionEntry_t )
	{
		assembleUsageString(currOptEntry, sizeof(usageString), usageString);

		printf("%-30s   %s" CXA_LINE_ENDING, usageString, (currOptEntry->description != NULL) ? currOptEntry->description : "<no description>");
	}
}


bool cxa_commandLineParser_parseOptions(cxa_commandLineParser_t *const clpIn, int argc, char* argv[])
{
	cxa_assert(clpIn);
	cxa_assert(argc > 0);
	cxa_assert(argv);

	// setup an array so we can keep track of which options are present
	cxa_array_t presentOptions;
	cxa_commandLineParser_optionEntry_t* presentOptions_raw[CXA_COMMAND_LINE_PARSER_MAX_NUM_OPTIONS];
	cxa_array_initStd(&presentOptions, presentOptions_raw);

	// iterate through our CL arguments
	for( int i = 1; i < argc; i++ )
	{
		char* passedOpt = argv[i];

		cxa_commandLineParser_optionEntry_t* expectedOpt = NULL;
		if( cxa_stringUtils_startsWith(passedOpt, "--") ) expectedOpt = findOptionByLongOpt(clpIn, passedOpt);
		else if( cxa_stringUtils_startsWith(passedOpt, "-") ) expectedOpt = findOptionByShortOpt(clpIn, passedOpt);
		else
		{
			fprintf(stderr, "Option [%s] not specified with preceding '--' or '-'" CXA_LINE_ENDING, passedOpt);
			return false;
		}

		// make sure we got our option
		if( expectedOpt == NULL )
		{
			fprintf(stderr, "Error: Unknown option [%s]" CXA_LINE_ENDING, passedOpt);
			return false;
		}
		if( !cxa_array_append(&presentOptions, &expectedOpt) ) return false;

		// now see if we need to parse an argument
		if( !expectedOpt->hasArgument )
		{
			// no argument...call our callback and move on to the next item
			if( expectedOpt->cb_noArg != NULL ) expectedOpt->cb_noArg(clpIn, expectedOpt->userVar);
			continue;
		}

		// if we made it here, we _may_ need to parse an argument
		if( expectedOpt->isArgumentRequired )
		{
			// make sure we actually have another element
			if( ++i >= argc )
			{
				fprintf(stderr, "Error: Required argument not specified for option [%s]" CXA_LINE_ENDING, passedOpt);
				return false;
			}
			// get the argument value
			cxa_stringUtils_parseResult_t argVal = cxa_stringUtils_parseString(passedOpt);
			if( argVal.dataType != expectedOpt->expectedArgType )
			{
				fprintf(stderr, "Error: Cannot parse expected %s argument from string [%s]" CXA_LINE_ENDING, cxa_stringUtils_getStringForDataType(argVal.dataType), passedOpt);
				return false;
			}

			// if we made it here, we have a valid argument...call our callback and move on to the next item
			if( expectedOpt->cb_arg != NULL ) expectedOpt->cb_arg(clpIn, &argVal, expectedOpt->userVar);
			continue;
		}
		else
		{
			// check to see if the next item is another option or our argument
			if( ++i >= argc )
			{
				// no argument
				if( expectedOpt->cb_arg != NULL ) expectedOpt->cb_arg(clpIn, NULL, expectedOpt->userVar);
				continue;
			}

			// if we made it ehre, things get a little tricky...
			char* nextStr = argv[i];
			if( cxa_stringUtils_startsWith(nextStr, "--") || cxa_stringUtils_startsWith(nextStr, "-") )
			{
				// the next string is an option...therefore we have no argument
				i--;
				if( expectedOpt->cb_arg != NULL ) expectedOpt->cb_arg(clpIn, NULL, expectedOpt->userVar);
				continue;
			}
			else
			{
				// the next string is not an option...therefore it should be an argument
				cxa_stringUtils_parseResult_t argVal = cxa_stringUtils_parseString(passedOpt);
				if( argVal.dataType != expectedOpt->expectedArgType )
				{
					fprintf(stderr, "Error: Cannot parse expected %s argument from string [%s]" CXA_LINE_ENDING, cxa_stringUtils_getStringForDataType(argVal.dataType), passedOpt);
					return false;
				}

				// if we made it here, we have a valid argument...call our callback and move on to the next item
				if( expectedOpt->cb_arg != NULL ) expectedOpt->cb_arg(clpIn, &argVal, expectedOpt->userVar);
				continue;
			}
		}
	}

	// if we made it here, we parsed everything successfully
	// now make sure we have all of our required options
	cxa_array_iterate( &clpIn->options, currOptEntry, cxa_commandLineParser_optionEntry_t )
	{
		if( currOptEntry->isRequired )
		{
			bool isOptPresent = false;
			cxa_array_iterate( &presentOptions, currPresentOptEntry, cxa_commandLineParser_optionEntry_t*)
			{
				if( (*currPresentOptEntry) == currOptEntry ) isOptPresent = true;
			}

			if( !isOptPresent )
			{
				fprintf(stderr, "Error: required argument not present [%s]" CXA_LINE_ENDING, (currOptEntry->shortOpt == NULL) ? currOptEntry->longOpt : currOptEntry->shortOpt);
				return false;
			}
		}
	}

	return true;
}


// ******** local function implementations ********
static void helpCallback(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	cxa_commandLineParser_printUsage(clpIn);
	exit(0);
}


static void assembleUsageString(cxa_commandLineParser_optionEntry_t *const optEntryIn, size_t maxStrLen_bytesIn, char *const stringOut)
{
	if( stringOut == NULL ) return;

	if( !optEntryIn->isRequired )
	{
		if( (optEntryIn->shortOpt == NULL) && (optEntryIn->longOpt != NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "[<--%s>%s]", optEntryIn->longOpt, (optEntryIn->hasArgument ? " <arg>]" : ""));
		}
		else if( (optEntryIn->shortOpt != NULL) && (optEntryIn->longOpt == NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "[<-%s>%s]", optEntryIn->shortOpt, (optEntryIn->hasArgument ? " <arg>]" : ""));
		}
		else if( (optEntryIn->shortOpt != NULL) && (optEntryIn->longOpt != NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "[<-%s | --%s>%s]", optEntryIn->shortOpt, optEntryIn->longOpt, (optEntryIn->hasArgument ? " <arg>]" : ""));
		}
	}
	else
	{
		if( (optEntryIn->shortOpt == NULL) && (optEntryIn->longOpt != NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "<--%s>%s", optEntryIn->longOpt, (optEntryIn->hasArgument ? " <arg>" : ""));
		}
		else if( (optEntryIn->shortOpt != NULL) && (optEntryIn->longOpt == NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "<-%s>%s", optEntryIn->shortOpt, (optEntryIn->hasArgument ? " <arg>" : ""));
		}
		else if( (optEntryIn->shortOpt != NULL) && (optEntryIn->longOpt != NULL) )
		{
			snprintf(stringOut, maxStrLen_bytesIn, "<-%s | --%s>%s", optEntryIn->shortOpt, optEntryIn->longOpt, (optEntryIn->hasArgument ? " <arg>" : ""));
		}
	}
}


static cxa_commandLineParser_optionEntry_t* findOptionByLongOpt(cxa_commandLineParser_t *const clpIn, char *passedOpt)
{
	cxa_assert(clpIn);
	cxa_assert(passedOpt);

	cxa_array_iterate(&clpIn->options, currOptEntry, cxa_commandLineParser_optionEntry_t)
	{
		if( currOptEntry->longOpt == NULL ) continue;

		// +2 to skip '--'
		if( strcmp(currOptEntry->longOpt, passedOpt+2) == 0 ) return currOptEntry;
	}

	return NULL;
}


static cxa_commandLineParser_optionEntry_t* findOptionByShortOpt(cxa_commandLineParser_t *const clpIn, char *passedOpt)
{
	cxa_assert(clpIn);
	cxa_assert(passedOpt);

	cxa_array_iterate(&clpIn->options, currOptEntry, cxa_commandLineParser_optionEntry_t)
	{
		if( currOptEntry->shortOpt == NULL ) continue;

		// +1 to skip '-'
		if( strcmp(currOptEntry->shortOpt, passedOpt+1) == 0 ) return currOptEntry;
	}

	return NULL;
}
