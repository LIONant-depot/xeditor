namespace xeditor::frame
{
    namespace details
    {
        constexpr auto main_frame_class_guid_v = frame::type_guid{ xeditor::frame::type_guid_v, xeditor::frame::main::class_name_v };

        template< typename T_TYPE, typename T_PARENT_TYPE = xeditor::frame::main >
        struct type_harness : type
        {
            constexpr static auto class_guid_v          = frame::type_guid{ xeditor::frame::type_guid_v, T_TYPE::class_name_v };
            constexpr static auto parent_class_guid_v   = frame::type_guid{ xeditor::frame::type_guid_v, T_PARENT_TYPE::class_name_v };

            constexpr type_harness( xcore::string::constant<char> Str, flags Flags = {}, int Weight = 0, xcore::icolor Col = xcore::icolor{ ~0u } ) noexcept
                : type
                (
                    []( xcore::string::constant<char> S ) constexpr noexcept -> xcore::string::constant<char>
                    {
                        std::array<char, 256> X{};
                        int i = 0, j = 0;
                        while (X[i] = "$xEditor/Tabs/"[i]) i++;
                        while (X[i + j] = S.m_pValue[j]) j++;

                        return xcore::string::constant<char>
                        { X.data() + 1
                        , xcore::string::constant<char>::units{0}
                        };
                    }(Str)
                    , Str
                    , class_guid_v
                    , parent_class_guid_v
                    , Flags
                    , Weight
                    , Col
                ) {}

            virtual std::unique_ptr<frame::base> New( xeditor::frame::main& EditorFrame ) const override
            {
                return std::make_unique<T_TYPE>( m_TypeName, type::CreateInstanceGuid(), EditorFrame );
            }
        };
    }

    //-------------------------------------------------------------------------------------------

    instance_guid type::CreateInstanceGuid(void) const
    {
        return instance_guid{ m_GuidSequence.m_Value++ };
    }

    //-------------------------------------------------------------------------------------------

    frame::type*& frame::type::getHead( void )
    { 
        static frame::type* pHead{ nullptr }; 
        return pHead; 
    }

    //-------------------------------------------------------------------------------------------

    xeditor::document::main& base::getMainDoc( void )
    { 
        return m_MainFrame.getMainDoc();
    }

    //-------------------------------------------------------------------------------------------
    bool base::isOpen(void) const noexcept
    { 
        return m_OpenPanel; 
    }
    //-------------------------------------------------------------------------------------------

    void base::setOpen(bool bOpen) 
    { 
        m_OpenPanel = bOpen; 
    }

    //-------------------------------------------------------------------------------------------

    bool base::isVisible(void) const noexcept 
    { 
        return m_OpenPanel && m_bPanelVisible; 
    }

    //-------------------------------------------------------------------------------------------

    void base::setActive(void) noexcept 
    { 
        m_bSetActive = true; 
    }

    //-------------------------------------------------------------------------------------------

    instance_guid base::getGuid(void) const noexcept 
    { 
        return m_InstanceGuid; 
    }

    //-------------------------------------------------------------------------------------------

    const ImGuiWindowClass& base::getImGuiClass( void ) const noexcept 
    { 
        return m_ImGuiClass; 
    }

}
