#pragma once

#include <stdio.h>

FILE *mypopen(const char *command, const char *type);

int mypclose(FILE *stream);