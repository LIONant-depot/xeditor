
//---------------------------------------------------------------------------------------------------------
inline
std::filesystem::path operator + ( const std::filesystem::path& A, const std::filesystem::path& B )
{
    std::filesystem::path temp{ A };
    temp += B;
    return temp;
}

//---------------------------------------------------------------------------------------------------------

namespace xeditor::document
{
    namespace path
    {
        template< typename T >
        using filepath = std::filesystem::path;

        using project           = filepath<struct tag_project>;
        using system            = filepath<struct tag_system>;
        using project_settings  = filepath<struct tag_project_settings>;
        using editor_settings   = filepath<struct tag_editor_settings>;
//        using plugins           = filepath<struct tag_plugins>;
        using qlion             = filepath<struct tag_qlion>;
    };

    //---------------------------------------------------------------------------------------------------------

    class main : public xeditor::document::base
    {
        xcore_rtti( main, xeditor::document::base )
    
    public:

        constexpr static auto           class_name_v = xcore::string::constant("xEditor/Documents/Main");

        struct events
        {
            using log_msg           = xcore::types::make_unique< xcore::event::type<const xcore::cstring&>, struct log_msg_tag >;
            using end_of_frame      = xcore::types::make_unique< xcore::event::type<>, struct end_of_frame_tag >;
            using close_project     = xcore::types::make_unique< xcore::event::type<>, struct close_project_tag >;
            using notify_ondelete   = xcore::types::make_unique< xcore::event::type<xcore::property::base&>, struct notify_ondelete_tag >;
            using notify_onchange   = xcore::types::make_unique< xcore::event::type<xcore::property::base&>, struct notify_onchange_tag >;

            log_msg             m_LogMsg;
            end_of_frame        m_EndOfFrame;
            close_project       m_CloseProject;
            notify_onchange     m_ChangeHappen;
            notify_ondelete     m_NotifyDelete;
        };

    public:

        events                                  m_Events;

    public:

                                                main                        ( xcore::string::constant<char> Str
                                                                            , xeditor::frame::base&         Frame )                 noexcept;
        inline                                  main                        ( xeditor::frame::base& Frame )                         noexcept;
        virtual                                ~main                        ( void )                                                noexcept;
        inline                  auto&           setupProjectFileExt         ( const path::project Extension )                       noexcept;
        
        xforceinline            void            setSystemDirectory          ( std::filesystem::path& Path )                         noexcept;
        xforceinline            const auto&     getSystemDirectory          ( void )                                                noexcept;
                                xcore::err      CreateNewProject            ( const path::project& ProjectPath )                    noexcept;
                                xcore::err      LoadProject                 ( const path::project& ProjectPath )                    noexcept;
        inline                  xcore::err      CloseProject                ( void )                                                noexcept;
        inline                  xcore::err      SaveProject                 ( void )                                                noexcept;
        xforceinline            const auto&     getCurrentPath              ( void )                                        const   noexcept;
        xforceinline            const auto&     getProjectPath              ( void )                                        const   noexcept;
        xforceinline            const auto&     getQLionPath                ( void )                                        const   noexcept;
        xforceinline            bool            isProjectEmpty              ( void )                                        const   noexcept;
                                void            NotifyChangeHappen          ( const xcore::cstring& Context, xcore::property::base& Prop ) noexcept;
                                void            NotifyDelete                ( const xcore::cstring& Context, xcore::property::base& Prop ) noexcept;
        virtual                 const type&     getType                     ( void )                                        const   noexcept;

                                xcore::err      CreateDir                   ( const std::filesystem::path& Path )           const   noexcept;
        xforceinline            auto&           getProjectSettingsPath      ( void )                                        const   noexcept;
        xforceinline            auto&           getEditorSettingsPath       ( void )                                        const   noexcept;
                                xcore::err      LoadEditorSettings          ( void )                                                noexcept;
                                xcore::err      SaveEditorSettings          ( void )                                                noexcept;
        xforceinline            auto&           getFrame                    ( void )                                                noexcept;
        template< typename T >
        inline                  T&              getSubDocument              ( void )                                                noexcept;

        template< typename T >
        inline                  const T&        getSubDocument              ( void )                                        const   noexcept;
        
    protected:

        struct table_entry 
        {
            base*   m_pBase;
            type*   m_pType;
        };

    protected:

        void                                msgGlobalLog            ( const xcore::cstring& Str )                                       noexcept;

                                xcore::err  SaveProjectSettings     ( void )                                                            noexcept;
                                xcore::err  LoadProjectSettings     ( void )                                                            noexcept;

        inline                  xcore::err  New                     ( void )                                                                    { xassert(false); return onNew();  }
                                void        StartUp                 ( const path::project& ProjectPath )                                noexcept;
        inline                  xcore::err  Load                    ( void )                                                                    { xassert(false); return onNew();  }
        inline                  xcore::err  Save                    ( void )                                                                    { xassert(false); return onNew();  }

        virtual                 xcore::err  onNew                   ( void )                                                            ;
        virtual                 xcore::err  onSave                  ( void )                                                            ;
        virtual                 xcore::err  onLoad                  ( void )                                                            ;
        virtual                 xcore::err  onClose                 ( void )                                                             override;
        virtual                 xcore::err  onSaveEditorSettings    ( void )                                                             override;
        virtual                 xcore::err  onLoadEditorSettings    ( void )                                                             override;

    protected:

        xeditor::frame::base&                           m_Frame;
        xcore::vector<table_entry>                      m_lxpSubEditors         {};
        events::log_msg::delegate                       m_GlobalLogDelegate     { this, &main::msgGlobalLog };

        path::project                                   m_ProjectExtension      {};     // folder extension of the folder/project name
        std::filesystem::path                           m_CurrentPath           {};     // Current path 
        path::project_settings                          m_ProjectSettingsPath   {};     // Project.extension/Settings
        path::editor_settings                           m_EditorSettingsPath    {};
        path::project                                   m_ProjectPath           {};     // Project.extension/
//        path::plugins                                   m_PluginsPath           {};
        path::qlion                                     m_QLionPath             {};
        path::system                                    m_SystemPath            {};
    };
}


