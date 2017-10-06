#ifndef helpers_h
#define helpers_h

// Prints an address in a human readable way. For debugging
void printAddress(const struct sockaddr_in* sockAddress, const socklen_t length);

// Converts a string in-place to all upper case
void stringToUpper(char* string, size_t length);

// Prints an error message for the given function that caused the error
void print_error(const char* error_function);

#endif // helpers_h
