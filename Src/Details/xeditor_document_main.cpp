namespace xeditor::document {

//---------------------------------------------------------------------------------------------------------
static main::events::log_msg s_eventGlobalLog;

void MyLogOutput( const xcore::log::channel& Channel, xcore::log::msg_type MsgType, const char* String, int Line, const char* file ) noexcept
{
    static constexpr auto   MsgTypeStr  = std::array{ "INFO", "WARNING", "ERROR" };
    static auto             Start       = std::chrono::high_resolution_clock::now();
    const auto              Duration    = std::chrono::high_resolution_clock::now() - Start;
    const float             MSTimer     = std::chrono::duration<float>(Duration).count();

    s_eventGlobalLog.NotifyAll( xcore::string::Fmt( "[%f][%s][%s] %s - (%s - %d)\n"
                                , MSTimer
                                , Channel.getName().c_str()
                                , MsgTypeStr[1+static_cast<int>(MsgType)]
                                , String
                                , file
                                , Line
                                ) 
                              );
}

//---------------------------------------------------------------------------------------------------------

void main::msgGlobalLog( const xcore::cstring& Str ) noexcept
{
    m_Events.m_LogMsg.NotifyAll(Str);
}

//---------------------------------------------------------------------------------------------------------

main::main( xcore::string::constant<char> Str, xeditor::frame::main& Frame ) noexcept
    : base( Str, *this )
    , m_Frame{ Frame }
{ 
    // Hook ourself to the global log pump
    m_GlobalLogDelegate.Connect( s_eventGlobalLog );
    xcore::get().m_MainLogger.m_pOutput = MyLogOutput;

    // Get the current path for reference
    {
        xcore::cstring CurPath;
        CurPath = xcore::string::To<char>(std::filesystem::current_path().c_str());
        xcore::string::CleanPath( CurPath );

        xcore::cstring EditorPath;
        xcore::string::Copy( EditorPath, CurPath );

        // Remove the (Bin) directory so that we are at the root of the editor path
        EditorPath[ xcore::cstring::units{ static_cast<std::uint32_t>(xcore::string::findLastInstance( EditorPath, '/' )) } ] = 0;
        xcore::string::CleanPath(EditorPath);

        xcore::cstring EditorSettingsPath = xcore::string::Fmt("%s/Config", EditorPath.data() );

        m_CurrentPath           = xcore::string::To<wchar_t>(CurPath).data();
        m_EditorSettingsPath    = xcore::string::To<wchar_t>(EditorSettingsPath).data();
    }

    //
    // Two pass creation 
    //

    // Allocate memory
    for( auto* pNext = type::getHead(); pNext; pNext = pNext->m_pNext )
    {
        if( pNext == &mains_type::s_Type ) continue;

        // we allocate the memory
        auto& Entry = m_lxpSubEditors.append();
        Entry.m_pBase = pNext->Malloc();
        Entry.m_pType = pNext;

        // set the first few bytes to zero it will get overriten by the virtual table later
        *reinterpret_cast<void**>(Entry.m_pBase) = nullptr; 
    }

    // Lets construct now
    for( auto& Entry : m_lxpSubEditors )
    {
        XLOG_CHANNEL_INFO( m_LogChannel, "Initializing: %s", Entry.m_pType->m_TypeName.m_pValue );
        Entry.m_pType->Construct( *Entry.m_pBase, Entry.m_pType->m_TypeName, *this );
        XLOG_CHANNEL_INFO( m_LogChannel, "Done Initializing: %s", Entry.m_pType->m_TypeName.m_pValue );
    }

    //
    // Init the frame officially
    //
    Frame._SystemMainInit( *this );
}

//---------------------------------------------------------------------------------------------------------

main::~main( void ) noexcept
{
    // Free any allocated entry
    for( auto& Entry : m_lxpSubEditors )
    {
        auto p = reinterpret_cast<void**>(Entry.m_pBase);
        xassume(p);

        // if it is constructed then use delete otherwise just free
        if( *p )
        {
            std::destroy_at( Entry.m_pBase );
        }

        xcore::memory::AlignedFree( Entry.m_pBase );
    }
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::SaveProjectSettings( void ) noexcept
{
    bool bError = false;
    auto Error = onSaveProjectSettings();

    if( Error )
    {
        bError = true; 
        Error.clear();
    }

    for( auto& EditorPair : m_lxpSubEditors )
    {
        auto Error = EditorPair.m_pBase->onSaveProjectSettings();
        if( Error )
        {
            bError = true; 
            Error.clear();
        }
    }

    if( bError ) return xerr_failure_s("Errors saving settings please check log");

    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::LoadProjectSettings( void ) noexcept
{
    bool bError = false;
    
    if( auto Error = onLoadProjectSettings(); Error )
    {
        bError = true; 
        Error.clear();
    }

    for( auto& EditorPair : m_lxpSubEditors )
    {
        if( auto Error = EditorPair.m_pBase->onLoadProjectSettings(); Error )
        {
            bError = true;
            Error.clear();
        }
    }

    if( bError ) return xerr_failure_s("Errors loading settings please check log");

    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::CreateDir( const std::filesystem::path& Path ) const noexcept
{
    std::error_code StdEC;
    if (create_directory(Path, StdEC) == false)
    {
        const auto Str = StdEC.message();
        XLOG_CHANNEL_ERROR(m_LogChannel, "System Error: %s", Str.c_str());
        return xerr_failure_s( "Fail to create a directory structure" );
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------

void main::StartUp( const path::project& ProjectPath ) noexcept
{
    m_ProjectSettingsPath   = ProjectPath; 
}

//---------------------------------------------------------------------------------------------------------

void main::NotifyChangeHappen( const xcore::cstring& Context, xcore::property::base& Prop ) noexcept
{ 
    XLOG_CHANNEL_INFO( m_LogChannel
        , Context.empty() ? "Change Happen but not valid context given" : static_cast<const char*>(Context)
    );

    m_Events.m_ChangeHappen.NotifyAll(Prop); 
}

//---------------------------------------------------------------------------------------------------------

void main::NotifyDelete( const xcore::cstring& Context, xcore::property::base& Prop ) noexcept
{ 
    XLOG_CHANNEL_INFO( m_LogChannel
        , Context.empty() ? "Deletion Happen but not valid context given" : static_cast<const char*>(Context)
    );

    m_Events.m_NotifyDelete.NotifyAll(Prop); 
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::LoadProject( const path::project& UsrProjectPath ) noexcept
{
    // Make sure that we have a close project
    if( auto Error = CloseProject(); Error ) return Error;

    //
    // lets do the loading part
    //
    if( false == m_ProjectExtension.empty() && std::wstring{ UsrProjectPath }.find( m_ProjectExtension ) == std::wstring::npos )
    {
        return xerr_failure_s( "Wrong extension for project" );
    }

    StartUp( UsrProjectPath );

    //
    // Setup the main path
    //
    m_ProjectPath.assign( UsrProjectPath );

    // Load the actual project
    if( auto Error = onLoad(); Error )
        return Error;

    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::CreateNewProject( const path::project& UsrProjectPath ) noexcept
{
    //
    // Create the main directory
    //
    auto ProjectPath = UsrProjectPath + m_ProjectExtension;
    StartUp( ProjectPath );

    //
    // Create the main directory
    //
    std::error_code ec;
    remove_all(ProjectPath, ec);
    xassert( !ec );
    
    if ( auto Error = CreateDir(ProjectPath); Error ) return Error;

    //
    // Create the setting direction
    //
    if( auto Error = CreateDir(m_ProjectSettingsPath); Error ) return Error;

    //
    // Finally copy the project path
    //
    m_ProjectPath = ProjectPath;

    //
    // Call onNew
    //
    if( auto Error = onNew(); Error ) return Error;

    return {};
}

//-------------------------------------------------------------------------------------------

xcore::err main::onSaveEditorSettings( void )
{
    bool bError = true;

    for (auto& Entry : m_lxpSubEditors)
    {
        try
        {
            if( auto Err = Entry.m_pBase->onSaveEditorSettings(); Err )
            {
                XLOG_CHANNEL_INFO(getMainDoc().m_LogChannel
                    , "ERROR: Fail to save editor settings for subeditor(%s) with error: %s"
                    , Entry.m_pType->m_TypeName.m_pValue
                    , Err.getCode().m_pString
                )
            }
            else
            {
                bError = false;
            }
        }
        catch( std::runtime_error e )
        {
            XLOG_CHANNEL_INFO(getMainDoc().m_LogChannel
                , "ERROR: Fail to save editor settings for subeditor(%s) with error: %s"
                , Entry.m_pType->m_TypeName.m_pValue
                , e.what()
            )
        }
        catch(...)
        {
            XLOG_CHANNEL_INFO(getMainDoc().m_LogChannel
                , "ERROR: Fail to save editor settings for subeditor(%s) with error: %s"
                , Entry.m_pType->m_TypeName.m_pValue
                , xcore::get().m_Scheduler.getWorkerExceptionInfo()
            )
        }
    }

    if (bError) return xerr_failure_s( "Fail to save the editor settings file" );

    return {};
}

//-------------------------------------------------------------------------------------------

xcore::err main::onLoadEditorSettings( void )
{
    bool bError = false;

    for (auto& Entry : m_lxpSubEditors)
    {
        try
        {
            if( auto Err = Entry.m_pBase->onLoadEditorSettings(); Err )
            {
                XLOG_CHANNEL_INFO(getMainDoc().m_LogChannel
                    , "ERROR: Fail to load editor settings for subeditor(%s) with error: %s"
                    , Entry.m_pType->m_TypeName.m_pValue
                    , Err.getCode().m_pString
                )

                    bError = true;
            }
        }
        catch(std::exception e)
        {
            XLOG_CHANNEL_INFO(getMainDoc().m_LogChannel
                , "ERROR: Fail to load editor settings for subeditor(%s) with error: %s"
                , Entry.m_pType->m_TypeName.m_pValue
                , xcore::get().m_Scheduler.getWorkerExceptionInfo()
            )

            bError = true;
        }
    }

    if (bError) return xerr_failure_s( "Fail to load the editor settings file" );

    return {};
}

//-------------------------------------------------------------------------------------------

xcore::err main::LoadEditorSettings(void) noexcept
{
    return onLoadEditorSettings();
}

//-------------------------------------------------------------------------------------------
xcore::err main::SaveEditorSettings(void) noexcept
{
    return onSaveEditorSettings();
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::onSave( void )
{
    //
    // Check if we have a project open
    //
    if( m_ProjectPath.empty() )
        return xerr_failure_s( "You don't have a project open yet" );

    //
    // Save settings
    //
    if( auto Error = SaveProjectSettings(); Error )
        return Error;

    //
    // Notify all editors that we are saving
    //
    bool bHadErrors = false;
    for( auto& EditorPair : m_lxpSubEditors )
    {
        auto& Editor = *EditorPair.m_pBase;

        if( auto Error = Editor.onSave(); Error )
        {
            bHadErrors = true;

             XLOG_CHANNEL_ERROR( m_LogChannel
                 , "Saving Error, Editor (%s) With Message (%s) "
                 , static_cast<const char*>( Editor.getType().m_TypeName.m_pValue )
                 , static_cast<const char*>( Error.getCode().m_pString) 
             );
            Error.clear();
        }
    }

    //
    // Error reporting
    //
    if( bHadErrors ) return xerr_failure_s( "We had errors while closing the project, please check the log" );
    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::onClose( void )
{
    //
    // Notify all editors that we are closing
    //
    bool bHadErrors = false;
    for( auto& EditorPair : m_lxpSubEditors )
    {
        auto& Editor = *EditorPair.m_pBase;

        if( auto Error = Editor.onClose(); Error )
        {
            bHadErrors = true;

             XLOG_CHANNEL_ERROR( m_LogChannel
                 , "Closing Error, Editor (%s) With Message (%s) "
                 , static_cast<const char*>( Editor.getType().m_TypeName.m_pValue )
                 , static_cast<const char*>( Error.getCode().m_pString) 
             );
        }
    }

    //
    // Reset members
    //
    m_ProjectPath.clear();
    m_ProjectSettingsPath.clear();

    //
    // Error reporting
    //
    if( bHadErrors ) return xerr_failure_s( "We had errors while Closing the project, please check the log" );
    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::onLoad( void )
{
    //
    // Save settings
    //
    if( auto Error = LoadProjectSettings(); Error )
        return Error;

    //
    // Notify all editors that we are saving
    //
    bool bHadErrors = false;
    for( auto& EditorPair : m_lxpSubEditors )
    {
        auto& Editor = *EditorPair.m_pBase;

        if( auto Error = Editor.onLoad(); Error )
        {
            bHadErrors = true;

             XLOG_CHANNEL_ERROR( m_LogChannel
                 , "Load Error, Editor (%s) With Message (%s) "
                 , static_cast<const char*>( Editor.getType().m_TypeName.m_pValue )
                 , static_cast<const char*>( Error.getCode().m_pString ) 
             );

            Error.clear();

            // if we had an error we need to close the project
            if (auto Err = m_MainDoc.CloseProject(); Err )
            {
                XLOG_CHANNEL_ERROR(m_LogChannel
                    , "Farther Load Errors, trying to Closing project now, Project, Editor (%s) With Message (%s) "
                    , static_cast<const char*>(Editor.getType().m_TypeName.m_pValue)
                    , static_cast<const char*>(Error.getCode().m_pString)
                );

                Err.clear();
            }
            break;
        }
    }

    //
    // Error reporting
    //
    if( bHadErrors ) return xerr_failure_s( "We had errors while Loading a project, please check the log" );
    return {};
}

//---------------------------------------------------------------------------------------------------------

xcore::err main::onNew( void )
{
    //
    // Notify all editors that we are saving
    //
    bool bHadErrors = false;
    for( auto& EditorPair : m_lxpSubEditors )
    {
        auto& Editor = *EditorPair.m_pBase;

        if( auto Error = Editor.onNew(); Error )
        {
            bHadErrors = true;

             XLOG_CHANNEL_ERROR( m_LogChannel
                 , "New Project Error, Editor (%s) With Message (%s) "
                 , static_cast<const char*>( Editor.getType().m_TypeName.m_pValue )
                 , static_cast<const char*>( Error.getCode().m_pString ) 
             );
            Error.clear();
        }
    }

    //
    // Error reporting
    //
    if( bHadErrors ) return xerr_failure_s( "We had errors while creating a new project, please check the log" );
    return {};
}



//---------------------------------------------------------------------------------------------------
// END
//---------------------------------------------------------------------------------------------------
} // namespace xeditor::document