namespace xeditor::panel
{
    namespace details
    {
        constexpr auto main_frame_class_guid_v = panel::type_guid{ xeditor::panel::type_guid_v, xeditor::frame::class_name_v };

        template< typename T_TYPE, typename...T_CHILDREN >
        struct type_harness : type
        {
            constexpr static auto class_guid_v          = panel::type_guid{ xeditor::panel::type_guid_v, T_TYPE::class_name_v };

            virtual std::unique_ptr<panel::parent> New(xeditor::frame& EditorFrame) const override
            {
                if constexpr (std::is_base_of_v< parent, T_TYPE>)
                {
                    auto Construct = parent::construct{ { m_TypeName, instance_guid{ type::CreateInstanceGuid().m_Value + class_guid_v.m_Value }, *this }, EditorFrame };

                    return std::make_unique<T_TYPE>(Construct);
                }
                else
                {
                    xassert(false);
                    return {};
                }
            }
            virtual std::unique_ptr<panel::child> New(panel::parent& Parent) const override
            {
                if constexpr (std::is_base_of_v< child, T_TYPE>)
                {
                    auto Construct = child::construct{ {m_TypeName, instance_guid{ type::CreateInstanceGuid().m_Value + class_guid_v.m_Value }, *this }, Parent };
                    return std::make_unique<T_TYPE>(Construct);
                }
                else
                {
                    xassert(false);
                    return {};
                }
            }
            
            constexpr type_harness( xcore::string::constant<char> Str, type::flags Flags = {}, int Weight = 0, xcore::icolor Col = xcore::icolor{ ~0u } ) noexcept
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
                    , Flags
                    , Weight
                    , Col
                ) 
                {
                    if constexpr ( std::is_base_of_v< panel::parent, T_TYPE > && sizeof...(T_CHILDREN))
                    {
                        auto Array = std::array
                        {
                            (type*)&T_CHILDREN::s_Type ...
                        };

                        for( int i=0u; i<(Array.size()-1); ++i )
                        {
                            Array[i]->m_pNext = Array[i+1];
                        }

                        m_pChildren = Array[0];
                    }
                }

        };
    }

    //-------------------------------------------------------------------------------------------

    instance_guid type::CreateInstanceGuid(void) const
    {
        return instance_guid{ m_GuidSequence.m_Value++ };
    }

    //-------------------------------------------------------------------------------------------

    panel::type*& panel::type::getHead( void )
    { 
        static panel::type* pHead{ nullptr }; 
        return pHead; 
    }

    //-------------------------------------------------------------------------------------------

    xeditor::document::main& parent::getMainDoc( void )
    { 
        return m_AppFrame.getMainDoc();
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

    const ImGuiWindowClass& parent::getImGuiClass( void ) const noexcept 
    { 
        return m_ImGuiClass; 
    }

}
