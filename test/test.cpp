#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    char buf[] = "delay 52\m30a0";
    int value = 0;
    int parse_result = 0; // 1 if an integer was parsed, 0 otherwise

    const char *prefix = "delay ";
    char *p = strstr(buf, prefix);

    if (p != NULL)
    {
        // Move past the prefix to the numeric part
        char *num_start = p + strlen(prefix);
        char *endptr = NULL;
        long v = strtol(num_start, &endptr, 10);

        if (endptr != num_start)
        {
            // Successfully parsed an integer prefix (e.g. "5230" from "5230a0")
            value = (int)v;
            parse_result = 1;
        }
        else
        {
            // No integer found after the prefix
            value = 0;
            parse_result = 0;
        }
    }
    else
    {
        // If the prefix isn't present, try to parse the whole buffer
        char *endptr = NULL;
        long v = strtol(buf, &endptr, 10);
        if (endptr != buf)
        {
            value = (int)v;
            parse_result = 1;
        }
        else
        {
            value = 0;
            parse_result = 0;
        }
    }

    printf("Parsed integer? %d, value = %d\n", parse_result, value);

    // Per request: return 0 if it is not possible to obtain an integer
    return 0;
}
