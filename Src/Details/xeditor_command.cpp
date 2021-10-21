#include <io.h>
#include <direct.h>

namespace xeditor::command {

//------------------------------------------------------------------------------

system::~system( void )
{
    Kill();
}

//------------------------------------------------------------------------------

void system::Init( std::int32_t UndoSteps, const char* pUndoPath )
{
    m_nUndoSystemSize = UndoSteps;
    m_pUndoSystemPath = xcore::string::Fmt("%s",pUndoPath);
    m_UndoSystem.Init(m_nUndoSystemSize, m_pUndoSystemPath);
}

//------------------------------------------------------------------------------

void system::Kill( void )
{
    m_UndoSystem.Kill();
}

//------------------------------------------------------------------------------

bool system::RegisterCommand( base& Command )
{
    // Register the command for current editor
    if ( !isCommandRegistered( Command ) )
    {
        m_CommandArray.append() = &Command;

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

bool system::ExecuteCommand( const char* pCmdline )
{
    xassert ( pCmdline );

    // Log
#ifdef _XCORE_DEBUG
    printf("[Execute Command] %s\n", pCmdline);
#endif

    // Skip spaces
    const int   nLength = xcore::string::Length(pCmdline).m_Value;
    int         iStart  = 0;
    while( (iStart < nLength) && xcore::string::isCharSpace(pCmdline[iStart]) ) 
    {
        ++iStart;
    }

    // Get command name and arguments from command line
    xcore::cstring CmdNameStr;
    xcore::cstring CmdlineStr;
    if ( iStart < nLength )
    {
        // When have command name
        int iEnd = iStart;
        while( (iEnd < nLength) && !xcore::string::isCharSpace(pCmdline[iEnd]) ) // find the end
        {
            ++iEnd;
        }

        xcore::string::CopyN(CmdNameStr, pCmdline + iStart, xcore::cstring::units{ static_cast<std::size_t>(iEnd - iStart) } );
        xcore::string::Copy(CmdlineStr, pCmdline + iEnd );

        // CmdNameStr.Copy( pCmdline + iStart, units_chars_any<xchar>{ units_bytes{ iEnd - iStart } } );
        // CmdlineStr.Copy(pCmdline + iEnd);
    }
    else
    {
        // When have no command name
        throw( std::runtime_error( "Error: Empty command line" ) );
        return false;
    }

    // Process for Help command
    if ( isHelpCommand(CmdNameStr) )
    {
        ExecuteHelpCommand(CmdlineStr);
        return true;
    }

    // Find command by name
    base* pCommand = findCommand(CmdNameStr);
    if ( !pCommand )
    {
        throw( std::runtime_error( xcore::string::Fmt("Error: Can not find command:[%s]", (const char*)CmdNameStr) ) );
        return false;
    }

    // Validate command line
    if ( !pCommand->ValidateCommand(CmdlineStr) )
    {
        throw(std::runtime_error(xcore::string::Fmt( "Error: Invalid command line:[%s%s]", (const char*)CmdNameStr, (const char*)CmdlineStr )));
        return false;
    }

    // Get new step from undo stack
    undo_step& Step = m_UndoSystem.getNewUndoStep();
    xcore::string::Copy(Step.m_Command, pCommand->getName());

    // Execute command and save undo/redo states to files
    pCommand->ExecuteCommand(Step);

    return true;
}

//------------------------------------------------------------------------------

bool system::Undo( void )
{
    // Undo command
    if ( m_UndoSystem.CanUndo() )
    {
        undo_step& Step = m_UndoSystem.UndoStep();
        base* pCommand = findCommand(Step.m_Command);
        pCommand->LoadUndoState(Step.m_UndoStateFile);

        // Log
#ifdef _XCORE_DEBUG
        printf("[Undo Command] %s\n", (const char*)Step.m_Command);
#endif

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

bool system::Redo( void )
{
    // Redo command
    if ( m_UndoSystem.CanRedo() )
    {
        undo_step& Step = m_UndoSystem.RedoStep();
        base* pCommand = findCommand(Step.m_Command);
        pCommand->LoadRedoState(Step.m_RedoStateFile);

        // Log
#ifdef _XCORE_DEBUG
        printf("[Redo Command] %s\n", (const char*)Step.m_Command);
#endif

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

bool system::isCommandRegistered( const base& Command ) const
{
    // Check if the command is already registered
    return findCommand(Command.getName()) != nullptr;
}

//------------------------------------------------------------------------------

base* system::findCommand( const char* pName ) const
{
    xassert ( pName );

    // Find the command by name
    for ( int i=0; i<m_CommandArray.size(); ++i )
    {
        if( xcore::string::Compare(m_CommandArray[i]->getName(), pName) == 0 )
        {
            return m_CommandArray[i];
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------------

bool system::CanUndo( void ) const
{
    return m_UndoSystem.CanUndo();
}

//------------------------------------------------------------------------------

bool system::CanRedo( void ) const
{
    return m_UndoSystem.CanRedo();
}

//------------------------------------------------------------------------------

bool system::isHelpCommand( const char* pName ) const
{
    xassert ( pName );

    if( xcore::string::Compare(pName, "Help") == 0 )
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

bool system::ExecuteHelpCommand( const char* pCmdline ) const
{
    xassert( pCmdline );

    xcore::cmdline::parser CmdLine;

    // Parse the command line
    if( auto Err = CmdLine.Parse( pCmdline ); Err )
    {
        throw(std::runtime_error(xcore::string::Fmt("Error: Parsing the command:[%s]", Err.getCode().m_pString )));
    }

    // Handle parameters
    if ( CmdLine.getCommandCount() == 1 )
    {
        base* pCommand = findCommand(CmdLine.getCommand(0).getName());
        if ( pCommand )
        {
            //printf("%s", pCommand->getHelp());
        }
        else
        {
            throw(std::runtime_error(xcore::string::Fmt("Error: Can not find this command:[%s]", (const char*)CmdLine.getCommand(0).getName() )));
            return false;
        }
    }
    else
    {
        throw(std::runtime_error(xcore::string::Fmt("Error: 1 argument is needed:[Help %s]", pCmdline)));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------

void system::InitUndoSystem(std::int32_t nUndoSize, const char* pUndoPath )
{
    // Make sure this function only be called once
    static bool s_bFlag = true;
    xassert ( s_bFlag );
    s_bFlag = false;

    xassert ( nUndoSize > 0 );
    xassert ( pUndoPath );
    xassert ( xcore::string::Length(pUndoPath).m_Value > 0 );

    m_nUndoSystemSize = nUndoSize;
    xcore::string::Copy(m_pUndoSystemPath, pUndoPath);

    // Make sure the end of path has one '/'
    auto iLast = xcore::string::Length(m_pUndoSystemPath).m_Value - 1;
    if ( m_pUndoSystemPath[iLast] != '\\' && m_pUndoSystemPath[iLast] != '/' )
    {
        xcore::string::Append( m_pUndoSystemPath, "/" );
    }

#ifdef _XCORE_PLATFORM_WINDOWS
    // Remove all undo files under tmp path
    _finddata_t filestruct;
    intptr_t handle = _findfirst( xcore::string::Fmt("%s*", m_pUndoSystemPath.data()).data(), &filestruct);
    if ( handle != -1 )
    {
        do
        {
            if ( ( filestruct.attrib & _A_SUBDIR ) == 0 )
            {
                remove( xcore::string::Fmt("%s%s", m_pUndoSystemPath.data(), filestruct.name ).data() );
            }
        } while ( _findnext(handle, &filestruct) != -1 );
    }
    _findclose(handle);

#elif defined _XCORE_PLATFORM_IOS || _XCORE_PLATFORM_ANDROID

#endif
}

//------------------------------------------------------------------------------

void system::Reset( void )
{
    m_UndoSystem.Reset();
}

//==============================================================================
//==============================================================================
//==============================================================================
// undo_system CLASS
//==============================================================================
//==============================================================================
//==============================================================================

//------------------------------------------------------------------------------

undo_system::~undo_system( void )
{
    //Kill();
}

//------------------------------------------------------------------------------

void undo_system::Init(std::int32_t nMaxStepCount, const char* pFilePath )
{
#define IS_SLASH(c)     ( (c) == '\\' || (c) == '/' )

    xassert ( nMaxStepCount > 0 );
    xassert ( pFilePath );
    xassert ( xcore::string::Length(pFilePath).m_Value > 0 );

    // Additionally allocate one node for end node when the stack is full
    if( auto Err = m_Stack.New( static_cast<std::size_t>(nMaxStepCount) + 1 ); Err ) throw(std::runtime_error( Err.getCode().m_pString ));

    m_iCurrent = 0;
    xcore::string::Copy(m_FilePath, pFilePath);

    // Make sure the end of path has one '/'
    if ( !IS_SLASH(m_FilePath[xcore::string::Length( m_FilePath ).m_Value -1]) )
    {
        xcore::string::Append( m_FilePath, "/");
    }

    // Create directory if it doesn't exist
    //if ( _mkdir(m_FilePath) == -1 && errno != EEXIST )
    //{
    //    char errorMsg[256];
    //    strerror_s(errorMsg, sizeof(errorMsg), errno);
    //    x_throw( xfs("%s:[%s]\n", errorMsg, m_FilePath) );
    //}
#ifdef _XCORE_PLATFORM_WINDOWS
    for ( int i=0; i<m_FilePath.size(); ++i )
    {
        if ( IS_SLASH(m_FilePath[i]) && !IS_SLASH(m_FilePath[i+1]) )
        {
            xcore::cstring strTmpPath;
            xcore::string::CopyN(strTmpPath, m_FilePath, xcore::cstring::units( static_cast<std::uint32_t>(i) ));
            _mkdir( strTmpPath );
        }
    }
#elif defined _XCORE_PLATFORM_IOS || _XCORE_PLATFORM_ANDROID

#endif
}

//------------------------------------------------------------------------------

void undo_system::Kill( void )
{
    // Delete all undo/redo files
    for ( int i=0; i<m_Stack.size(); ++i )
    {
        DeleteFiles(m_Stack[i]);
    }
}

//------------------------------------------------------------------------------

void undo_system::Reset( void )
{
    // Store initialization status
    int             nMaxStepCount = m_Stack.size<int>() - 1;
    xcore::cstring  FilePath(m_FilePath);

    // Kill and Init
    Kill();
    Init(nMaxStepCount, FilePath);
}

//------------------------------------------------------------------------------

bool undo_system::CanUndo(std::int32_t nSteps /*= 1 */ ) const
{
    xassert ( nSteps > 0 );

    int iPrevStep = m_Stack[getPrev()].m_iStep;

    // If previous step is -1, means no steps before current step(can't undo)
    if ( iPrevStep < 0 )
    {
        return false;
    }

    // For single undo, already checked previous step so return true
    if ( nSteps == 1 )
    {
        return true;
    }

    // For multiple undo, go through the stack nodes to get the available undo sum
    int nSum = 0;
    for (int i=0; i<m_Stack.size(); ++i )
    {
        if ( m_Stack[i].m_iStep >= 0 && m_Stack[i].m_iStep <= iPrevStep )
        {
            ++nSum;
        }
    }
    return ( nSum >= nSteps );
}

//------------------------------------------------------------------------------

bool undo_system::CanRedo(std::int32_t nSteps /*= 1 */ ) const
{
    xassert ( nSteps > 0 );

    int iCurStep = m_Stack[m_iCurrent].m_iStep;

    // If current step is -1, means no steps after previous step(can't redo)
    if ( iCurStep < 0 )
    {
        return false;
    }

    // For single redo, already checked current step so return true
    if ( nSteps == 1 )
    {
        return true;
    }

    // For multiple redo, go through the stack nodes to get the available redo sum
    int nSum = 0;
    for ( int i=0; i<m_Stack.size(); ++i )
    {
        if ( m_Stack[i].m_iStep >= iCurStep )
        {
            ++nSum;
        }
    }
    return ( nSum >= nSteps );
}

//------------------------------------------------------------------------------

undo_step& undo_system::UndoStep( void )
{
    //x_assert ( CanUndo() );

    // Return previous node and set it as current node
    undo_step& Step = m_Stack[getPrev()];
    m_iCurrent = getPrev();

    // Make sure the file pointer is at the beginning
    if( auto Err = Step.m_UndoStateFile.SeekOrigin( xcore::units::bytes{0u}); Err ) throw(std::runtime_error("Error: Failed seek"));

    return Step;
}

//------------------------------------------------------------------------------

undo_step& undo_system::RedoStep( void )
{
    //x_assert ( CanRedo() );

    // Return current node and set next as current node
    undo_step& Step = m_Stack[m_iCurrent];
    m_iCurrent = getNext();

    // Make sure the file pointer is at the beginning
    if (auto Err = Step.m_RedoStateFile.SeekOrigin(xcore::units::bytes{ 0u }); Err) throw(std::runtime_error("Error: Failed seek (2)"));

    return Step;
}

//------------------------------------------------------------------------------

undo_step& undo_system::getNewUndoStep( void )
{
    int iPrevStep = m_Stack[getPrev()].m_iStep;

    // Clear unused nodes after current step
    for ( int i=0; i<m_Stack.size(); ++i )
    {
        if ( m_Stack[i].m_iStep > iPrevStep )
        {
            ResetStep(m_Stack[i]);
        }
    }

    // Initialize the new node (MUST be here)
    undo_step& Step = m_Stack[m_iCurrent];
    Step.m_iStep = iPrevStep + 1;
    CreateFiles(Step);


    // Move current index to next node and make sure current node is cleared (means it's the end of stack)
    m_iCurrent = getNext();
    ResetStep(m_Stack[m_iCurrent]);

    return Step;
}

//------------------------------------------------------------------------------

std::int32_t undo_system::getPrev( void ) const
{
    // Check and return the previous node's index
    int iPrev = m_iCurrent - 1;
    if ( iPrev < 0 )
    {
        iPrev = m_Stack.size<int>() - 1;
    }
    return iPrev;
}

//------------------------------------------------------------------------------

std::int32_t undo_system::getNext( void ) const
{
    // Check and return the next node's index
    int iNext = m_iCurrent + 1;
    if ( iNext >= m_Stack.size<int>() )
    {
        iNext = 0;
    }
    return iNext;
}

//------------------------------------------------------------------------------

xcore::cstring undo_system::getUndoFileName( std::int32_t iStep ) const
{
    return xcore::string::Fmt("%s%p_UNDO_%d.TMP", m_FilePath.data(), (void*)this, iStep);
}

//------------------------------------------------------------------------------

xcore::cstring undo_system::getRedoFileName(std::int32_t iStep ) const
{
    return xcore::string::Fmt("%s%p_REDO_%d.TMP", m_FilePath.data(), (void*)this, iStep);
}

//------------------------------------------------------------------------------

void undo_system::CreateFiles( undo_step& Step )
{
    // Only available for active step
    xassert ( Step.m_iStep >= 0 );

    // Make sure clossed the old files and cleared the xfiles before reopen
    Step.m_UndoStateFile.close();
    Step.m_RedoStateFile.close();

    // Create files for both read and write
    if ( auto Err = Step.m_UndoStateFile.open(xcore::string::To<wchar_t>(getUndoFileName(Step.m_iStep)), "w"); Err )
    {
        throw(std::runtime_error(xcore::string::Fmt("Error: Can not create undo file:[%s] (%s)"
                 , (const char*)getUndoFileName(Step.m_iStep)
                 , Err.getCode().m_pString )));
        return;
    }
    if ( auto Err = Step.m_RedoStateFile.open(xcore::string::To<wchar_t>(getRedoFileName(Step.m_iStep)), "w"); Err )
    {
        throw(std::runtime_error(xcore::string::Fmt( "Error: Can not create redo file:[%s] (%s)"
                , (const char*)getRedoFileName(Step.m_iStep)
                , Err.getCode().m_pString)));
        return;
    }

    // Force flush
    Step.m_UndoStateFile.Flush();
    Step.m_RedoStateFile.Flush();

    // Make sure the file pointers are at the beginning
    if( auto Err = Step.m_UndoStateFile.SeekOrigin(xcore::units::bytes{ 0u }); Err) throw(std::runtime_error("Error: Failed seek (3)"));
    if (auto Err = Step.m_RedoStateFile.SeekOrigin(xcore::units::bytes{ 0u }); Err) throw(std::runtime_error("Error: Failed seek (4)"));

}

//------------------------------------------------------------------------------

void undo_system::DeleteFiles( undo_step& Step )
{
    // If current step is not active, DeleteFile will do nothing
    if ( Step.m_iStep < 0 )
    {
        return;
    }

    // Make sure clossed the old files and cleared the xfiles before delete
    Step.m_UndoStateFile.close();
    Step.m_RedoStateFile.close();

    // Delete files
    remove(getUndoFileName(Step.m_iStep));
    remove(getRedoFileName(Step.m_iStep));

}

//------------------------------------------------------------------------------

void undo_system::ResetStep( undo_step& Step )
{
    // Delete files and reset the step status
    DeleteFiles(Step);
    Step.Clear();
}

//==============================================================================
//==============================================================================
//==============================================================================
// undo_step STRUCT
//==============================================================================
//==============================================================================
//==============================================================================

//------------------------------------------------------------------------------

undo_step::undo_step( void )
{
    Clear();
}

//------------------------------------------------------------------------------

undo_step::~undo_step( void )
{

}

//------------------------------------------------------------------------------

void undo_step::Clear( void )
{
    m_iStep = -1;
    m_Command.clear();
    m_UndoStateFile.close();
    m_RedoStateFile.close();
}

//==============================================================================
//==============================================================================
//==============================================================================
// base CLASS
//==============================================================================
//==============================================================================
//==============================================================================

//------------------------------------------------------------------------------

base::~base( void )
{

}


    //---------------------------------------------------------------------------------------------------
    // END
    //---------------------------------------------------------------------------------------------------
} // namespace xeditor::command 