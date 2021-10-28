namespace xeditor
{
    //---------------------------------------------------------------------------------------------------------

    xeditor::document::main& frame::getMainDoc(void) noexcept
    { 
        return xcore::rtti::SafeCast<xeditor::document::main>( *m_pDocument );
    }

    //---------------------------------------------------------------------------------------------------------

    auto& frame::getTabList(void) noexcept
    { 
        return m_lPanels;
    }

    //---------------------------------------------------------------------------------------------------------

    xgpu::window& frame::getWindow(void) noexcept
    { 
        return m_XGPUWindow; 
    }

    //---------------------------------------------------------------------------------------------------------

    void frame::Awake(void) noexcept
    { 
        onAwake(); 
    }
    
    //---------------------------------------------------------------------------------------------------------

    template< typename T >
    void frame::appendDelayCmd(T&& Function) noexcept
    { 
        m_DelayCmds.append(std::forward<T&&>(Function)); 
    }

    //---------------------------------------------------------------------------------------------------------
    inline
    void frame::onAwake(void)
    { 
        m_CoolDown = 10; 
    }

    //---------------------------------------------------------------------------------------------------------

    const ImGuiWindowClass& frame::getImGuiClass(void) const noexcept
    { 
        return m_ImGuiClass; 
    }

    //---------------------------------------------------------------------------------------------------------
    const ImGuiWindowClass& frame::getImGuiClass(panel::instance_guid ParentGuid) const noexcept
    {
        for (auto& E : m_lPanels )
        {
            if( ParentGuid == E->getGuid() )
            {
                return E->getImGuiClass();
            }
        }
        xassume(false);
        static ImGuiWindowClass x {};
        return x;
    }

}