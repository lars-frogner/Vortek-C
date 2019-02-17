#ifndef ERROR_H
#define ERROR_H

void print_info_message(const char* message, ...);
void print_warning_message(const char* message, ...);
void print_error_message(const char* message, ...);
void print_severe_message(const char* message, ...);


// Define check statement (always active)
#define check(expression) \
    ((expression)? \
        (void)0 : \
        print_severe_message("check \"%s\" failed in %s, line %d", \
                             #expression, __FILE__, __LINE__))


// Define assert statement (only active in debug mode)
#ifdef NDEBUG
    #define assert(expression) ((void)0)
#else
    #define assert(expression) \
        ((expression)? \
            (void)0 : \
            print_severe_message("assertion \"%s\" failed in %s, line %d", \
                                 #expression, __FILE__, __LINE__))
#endif


#endif
