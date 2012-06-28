#include "csimplescan.h"

CSimpleScan::CSimpleScan()
{
	m_bInterfaceSet = false;
}

CSimpleScan::CSimpleScan( const char *filename )
{
	SetModule( filename );
}

bool CSimpleScan::SetModule( const char *filename )
{
	m_Handle = GetModuleHandle(filename);

	CSigScan::module_handle = m_Handle;

	if ( !CSigScan::GetModuleMemInfo() )
		return m_bInterfaceSet = false;

	m_bInterfaceSet = ( m_Handle != NULL );

	return m_bInterfaceSet;
}

bool CSimpleScan::FindFunction( const char *sig, const char *mask, void **func )
{
	if ( !m_bInterfaceSet )
		return false;

	
	m_Signature.Init( ( unsigned char * )sig, ( char * )mask, strlen( mask ) );

	if ( !m_Signature.is_set )
		return false;

	*func = m_Signature.sig_addr;

	return true;
}