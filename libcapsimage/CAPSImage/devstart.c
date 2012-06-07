#include <exec/types.h>

/* prevent crash when started from CLI */
static LONG __attribute__((used)) DevStart(VOID)
{
	return -1;
}

