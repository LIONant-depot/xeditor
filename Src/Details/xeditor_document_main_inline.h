namespace xeditor::document  // main
{
    //---------------------------------------------------------------------------------------------------------

    main::main( xeditor::frame::base& Frame ) noexcept 
        : main(class_name_v, Frame )
    {}

    //---------------------------------------------------------------------------------------------------------

    auto& main::setupProjectFileExt( const path::project Extension ) noexcept 
    { 
        m_ProjectExtension.assign( Extension ); 
        return *this; 
    }

    //---------------------------------------------------------------------------------------------------------

    void main::setSystemDirectory( std::filesystem::path& Path ) noexcept 
    { 
        m_SystemPath = Path; 
    }

    //---------------------------------------------------------------------------------------------------------

    const auto& main::getSystemDirectory( void ) noexcept 
    { 
        return m_SystemPath; 
    }

    //---------------------------------------------------------------------------------------------------------

    xcore::err main::CloseProject( void ) noexcept 
    { 
        m_Events.m_CloseProject.NotifyAll(); 
        return onClose(); 
    }

    //---------------------------------------------------------------------------------------------------------

    xcore::err main::SaveProject ( void ) noexcept 
    { 
        return onSave(); 
    }

    //---------------------------------------------------------------------------------------------------------

    const auto& main::getCurrentPath( void ) const noexcept 
    { 
        return m_CurrentPath; 
    }

    //---------------------------------------------------------------------------------------------------------

    const auto& main::getProjectPath ( void ) const   noexcept 
    { 
        return m_ProjectPath;
    }

    //---------------------------------------------------------------------------------------------------------

    const auto& main::getQLionPath( void ) const noexcept 
    { 
        return m_QLionPath;   
    }

    //---------------------------------------------------------------------------------------------------------

    bool main::isProjectEmpty( void ) const noexcept 
    { 
        return m_ProjectPath.empty(); 
    }

    //---------------------------------------------------------------------------------------------------------

    auto& main::getProjectSettingsPath ( void ) const   noexcept 
    { 
        return m_ProjectSettingsPath;     
    }

    //---------------------------------------------------------------------------------------------------------

    auto& main::getEditorSettingsPath ( void ) const   noexcept 
    { 
        return m_EditorSettingsPath; 
    }

    //---------------------------------------------------------------------------------------------------------

    auto& main::getFrame( void ) noexcept 
    { 
        return m_Frame; 
    }

    //---------------------------------------------------------------------------------------------------------

    template< typename T >
    T& main::getSubDocument(void) noexcept
    {
        constexpr static auto class_guid_v = guid{ xeditor::document::type_guid_v, T::class_name_v };

        for (auto& EditorPair : m_lxpSubEditors)
        {
            auto& Type = *EditorPair.m_pType;
            if (class_guid_v == Type.getGuid())
            {
                return *reinterpret_cast<T*>(EditorPair.m_pBase);
            }
        }

        xassume(false);
        return *reinterpret_cast<T*>(nullptr);
    }

    //---------------------------------------------------------------------------------------------------------

    template< typename T >
    const T& main::getSubDocument(void) const noexcept
    { 
        return const_cast<main*>(this)->getSubDocument<T>(); 
    }

}