
/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging configurations in the 
 * following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the logging configurations for your application. It is required
 * to define LIBRARY_LOG_NAME, LIBRARY_LOG_LEVEL and SdkLog macros.
 * 3. Include the header file "logging_stack.h".
 */

#include "logging_levels.h"

/* Logging configurations for the application. */

/* Set the application log name. */
#ifndef LIBRARY_LOG_NAME
        #define LIBRARY_LOG_NAME "Views"
#endif 

/* Set the logging verbosity level. */
#ifndef LIBRARY_LOG_LEVEL
        #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif 

/* Define the metadata information to add in each log.
 * The example here sets the metadata to [:]. */
#if !defined( LOG_METADATA_FORMAT ) && !defined( LOG_METADATA_ARGS )
   #define LOG_METADATA_FORMAT "[%s:%d]"
   #define LOG_METADATA_ARGS __FILE__, __LINE__
#endif

/* Define the platform-specific logging function to call from
 * enabled logging macros. */
#ifndef SdkLog
    #define SdkLog( message )   printf(message)
#endif

/************ End of logging configuration ****************/
