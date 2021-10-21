namespace xeditor::document // base
{
    namespace details
    {
        template< typename T_TYPE >
        struct type_harness : type
        {
            constexpr static auto class_guid_v = guid{ xeditor::document::type_guid_v, T_TYPE::class_name_v };

            type_harness( xcore::string::constant<char> Str, int Weight = 0 ) noexcept
                : type(Str, class_guid_v, Weight )
                {}

            virtual void Construct ( base& Base, xcore::string::constant<char> Str, main& MainDoc ) const noexcept override
            {
                if constexpr (std::is_same< T_TYPE, main >::value || std::is_base_of< main, T_TYPE>::value)
                {
                    xassert(false);
                }
                else
                {
                    new(&Base) T_TYPE(Str, MainDoc);
                }
            }

            virtual base* Malloc ( void ) const noexcept override
            {
                if constexpr (std::is_same< T_TYPE, main >::value || std::is_base_of< main, T_TYPE>::value)
                {
                    xassert(false);
                    return nullptr;
                }
                else
                {
                    return reinterpret_cast<base*>( xcore::memory::AlignedMalloc( xcore::units::bytes{ sizeof(T_TYPE) }, alignof(T_TYPE) ) );
                }
            }
        };
    }

    //---------------------------------------------------------------------------------------------------

    xeditor::document::main& base::getMainDoc(void) noexcept
    { 
        return xcore::rtti::SafeCast<main>(m_MainDoc);
    }

    //---------------------------------------------------------------------------------------------------

    const xeditor::document::main& base::getMainDoc(void) const noexcept
    { 
        return xcore::rtti::SafeCast<main>(m_MainDoc);
    }
}
