#include <string.h>
#include <stddef.h>

char * strtok_r (char *s, const char *delim, char **save_ptr)
{
  char *end;
 
  if (s == NULL)
    s = *save_ptr;
 
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }
 
  /* Scan leading delimiters.  */
  s += strspn (s, delim);
  if (*s == '\0')
    {
      *save_ptr = s;
      return NULL;
    }
 
  /* Find the end of the token.  */
  end = s + strcspn (s, delim);
  if (*end == '\0')
    {
      *save_ptr = end;
      return s;
    }
 
  /* Terminate the token and make *SAVE_PTR point past it.  */
  *end = '\0';
  *save_ptr = end + 1;
  return s;
}
