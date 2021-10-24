namespace xeditor::frame
{
    //---------------------------------------------------------------------------------------------------------

    xeditor::document::main& main::getMainDoc(void) noexcept
    { 
        return xcore::rtti::SafeCast<xeditor::document::main>( *m_pDocument );
    }

    //---------------------------------------------------------------------------------------------------------

    auto& main::getTabList(void) noexcept
    { 
        return m_lFrames;
    }

    //---------------------------------------------------------------------------------------------------------

    xgpu::window& main::getWindow(void) noexcept
    { 
        return m_XGPUWindow; 
    }

    //---------------------------------------------------------------------------------------------------------

    void main::Awake(void) noexcept
    { 
        onAwake(); 
    }
    
    //---------------------------------------------------------------------------------------------------------

    template< typename T >
    void main::appendDelayCmd(T&& Function) noexcept
    { 
        m_DelayCmds.append(std::forward<T&&>(Function)); 
    }

    //---------------------------------------------------------------------------------------------------------
    inline
    void main::onAwake(void)
    { 
        m_CoolDown = 10; 
    }

    //---------------------------------------------------------------------------------------------------------
    const ImGuiWindowClass& main::getImGuiClass(frame::instance_guid ParentGuid) const noexcept
    {
        for (auto& E : m_lFrames)
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