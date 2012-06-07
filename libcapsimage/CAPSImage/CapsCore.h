#ifndef CAPSCORE_H
#define CAPSCORE_H

#define CAPS_LIB_RELEASE  4
#define CAPS_LIB_REVISION 2


void CAPSLockTrackT0(PCAPSTRACKINFO pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag);
void CAPSLockTrackT1(PCAPSTRACKINFOT1 pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag);
void CAPSLockTrackT2(PCAPSTRACKINFOT2 pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag);
void CAPSGetVersionInfoT0(PCAPSVERSIONINFO pi);
int CAPSGetSectorInfo(PCAPSSECTORINFO pi, PDISKTRACKINFO pt, UDWORD infid);
int CAPSGetWeakInfo(PCAPSDATAINFO pd, PDISKTRACKINFO pt, UDWORD infid);


typedef CTypedPtrArray<CPtrArray, PCDISKIMAGE> CDiskImagePtrArray;

#endif