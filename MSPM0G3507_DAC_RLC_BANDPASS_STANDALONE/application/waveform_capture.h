#ifndef WAVEFORM_CAPTURE_H
#define WAVEFORM_CAPTURE_H

#include <stdbool.h>

void Waveform_PrintAscii(void);
void Waveform_PrintCsv(void);
void Waveform_VofaStart(void);
void Waveform_VofaStop(void);
void Waveform_VofaProc(void);
bool Waveform_IsVofaStreaming(void);

#endif
