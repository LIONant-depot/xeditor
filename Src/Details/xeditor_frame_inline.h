namespace xeditor::frame
{
    //---------------------------------------------------------------------------------------------------------

    xeditor::document::main& base::getMainDoc(void) noexcept
    { 
        return xcore::rtti::SafeCast<xeditor::document::main>( *m_pDocument );
    }

    //---------------------------------------------------------------------------------------------------------

    auto& base::getTabList(void) noexcept
    { 
        return m_lTab;
    }

    //---------------------------------------------------------------------------------------------------------

    auto base::getWindow(void) noexcept
    { 
        return m_pWindow; 
    }

    //---------------------------------------------------------------------------------------------------------

    void base::Awake(void) noexcept
    { 
        onAwake(); 
    }
    
    //---------------------------------------------------------------------------------------------------------

    template< typename T >
    void base::appendDelayCmd(T&& Function) noexcept
    { 
        m_DelayCmds.append(std::forward<T&&>(Function)); 
    }

    //---------------------------------------------------------------------------------------------------------
    inline
    void base::onAwake(void)
    { 
        m_CoolDown = 10; 
    }
}