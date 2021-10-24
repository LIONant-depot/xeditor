namespace xeditor::tab
{
    namespace details
    {
        template< typename T_TYPE >
        struct type_harness : type
        {
            constexpr static auto class_guid_v = tab::type_guid{ xeditor::tab::type_guid_v, T_TYPE::class_name_v };

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
                    , Flags
                    , Weight
                    , Col
                ) {}

            virtual std::unique_ptr<tab::base> New( xeditor::frame::base& EditorFrame ) const override
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

    tab::type*& tab::type::getHead( void )
    { 
        static tab::type* pHead{ nullptr }; 
        return pHead; 
    }

    //-------------------------------------------------------------------------------------------

    xeditor::document::main& base::getMainDoc( void )
    { 
        return m_EditorFrame.getMainDoc();
    }
}
