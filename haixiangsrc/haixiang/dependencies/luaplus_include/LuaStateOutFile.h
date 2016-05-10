///////////////////////////////////////////////////////////////////////////////
// This source file is part of the LuaPlus source distribution and is Copyright
// 2001-2004 by Joshua C. Jensen (jjensen@workspacewhiz.com).
//
// The latest version may be obtained from http://wwhiz.com/LuaPlus/.
//
// The code presented in this file may be used in any environment it is
// acceptable to use Lua.
///////////////////////////////////////////////////////////////////////////////
#ifndef LUASTATEOUTFILE_H
#define LUASTATEOUTFILE_H

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// namespace LuaPlus
///////////////////////////////////////////////////////////////////////////////
namespace LuaPlus
{

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Output file helper class.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/**
	The DumpObject() facility uses a LuaStateOutFile derived class to
	output data to.  The LuaStateOutFile class may be derived from to enable
	an application specific method of output.
**/
class LUAPLUS_CLASS LuaStateOutFile
{
public:
	LuaStateOutFile() : m_file( NULL ), m_fileOwner( false ) {}
	LuaStateOutFile(const char* fileName) : m_file( NULL ), m_fileOwner( false )
	{
		Open(fileName);
	}

	virtual ~LuaStateOutFile()
	{
		if ( m_file  &&  m_fileOwner )
			fclose( m_file );
	}

	virtual bool Open( const char* fileName )
	{
		Close();

		m_file = fopen( fileName, "wb" );
		m_fileOwner = true;

		return m_file != NULL;
	}

	virtual void Close()
	{
		if ( m_file  &&  m_fileOwner )
			fclose( m_file );
	}

	virtual void Print( const char* str, ... )
	{
		char message[ 800 ];
		va_list arglist;

		va_start( arglist, str );
		vsprintf( message, str, arglist );
		va_end( arglist );

		fputs( message, m_file );
	}

	bool Assign( FILE* file )
	{
		m_file = file;
		m_fileOwner = false;

		return true;
	}
	
	void Indent( unsigned int indentLevel );

protected:
	FILE* m_file;
	bool m_fileOwner;
};

} // namespace LuaPlus

#endif