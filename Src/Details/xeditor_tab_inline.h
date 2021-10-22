namespace xeditor::tab
{
    namespace details
    {
        template< typename T_TYPE >
        struct type_harness : type
        {
            constexpr type_harness(const xcore::string::constant<char> Str, flags Flags = {}, int Weight = 0, xcore::icolor Col = xcore::icolor{ ~0u } ) noexcept
                : type
                (
                    []( const xcore::string::constant<char> S ) noexcept
                    {
                        static std::array<char, 256> X;
                        int i = 0, j = 0;
                        while (X[i] = "$gbEditor/Tabs/"[i]) i++;
                        while (X[i + j] = S.m_pValue[j]) j++;
                        return xcore::string::constant<char>{ &X[1] };
                    }(Str)
                    , Str
                    , Flags
                    , Weight
                    , Col
                ) {}

            virtual std::unique_ptr<tab::base> New( xeditor::frame::base& EditorFrame ) const override
            {
                return std::make_unique<T_TYPE>( m_TypeName, EditorFrame ).release();
            }
        };
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
