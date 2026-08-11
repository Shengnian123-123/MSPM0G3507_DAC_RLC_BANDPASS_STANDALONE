#ifndef APP_SIGNAL_TOOLS_LAB_H
#define APP_SIGNAL_TOOLS_LAB_H

#include <stdbool.h>

void SignalToolsLab_Init(void);
void SignalToolsLab_WriteHelp(void);
bool SignalToolsLab_HandleCommand(const char *line);
void SignalToolsLab_Proc(void);
void SignalToolsLab_StopStreaming(void);
bool SignalToolsLab_IsStreaming(void);

#endif
