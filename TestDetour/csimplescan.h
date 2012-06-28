#ifndef CSIMPLESCAN_H
#define CSIMPLESCAN_H

#include "sigscan.h"

class CSimpleScan
{

public:
	CSimpleScan();
	CSimpleScan( const char *filename );

	bool SetModule( const char *filename );
	bool FindFunction( const char *sig, const char *mask, void **func );

private:
	bool m_bInterfaceSet;
	bool m_bDllInfo;

	void* m_Handle;
	CSigScan m_Signature;
};

#endif //CSIMPLESCAN_H