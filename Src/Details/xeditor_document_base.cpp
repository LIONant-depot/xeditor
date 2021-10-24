//-------------------------------------------------------------------------------------------
//  Properties for the base doc must be in a globa scope
//-------------------------------------------------------------------------------------------
property_begin_name(xeditor::document::base, "BaseDocument")
{
    property_var_fnbegin("Name", xcore::cstring)
    {
        if (isRead) xcore::string::Copy(InOut, Self.getType().m_TypeName);
    } property_var_fnend()
        .Flags(property::flags::SHOW_READONLY | property::flags::DONTSAVE)
        .Help("Name of the document")

,   property_var_fnbegin("Weight", int)
    {
        if (isRead) InOut = Self.getType().m_Weight;
    } property_var_fnend()
        .Flags(property::flags::SHOW_READONLY | property::flags::DONTSAVE)
        .Help("Determines the execution order")

} property_vend_cpp(xeditor::document::base)

//-------------------------------------------------------------------------------------------
// Back to the expected CPP
//-------------------------------------------------------------------------------------------
namespace xeditor::document {

//-------------------------------------------------------------------------------------------

namespace mains_type
{
    static const details::type_harness<main> s_Type{ main::class_name_v, -100 };
}

//-------------------------------------------------------------------------------------------

const type& main::getType( void ) const noexcept
{ 
    return mains_type::s_Type;  
}

//-------------------------------------------------------------------------------------------

type::type( xcore::string::constant<char> Str, type_guid ClassGuid, int Weight ) noexcept
    : m_TypeName    { Str       }
    , m_Weight      { Weight    }
    , m_TypeGuid        { ClassGuid }
    , m_pNext       { nullptr   }
{ 
    if( getHead() == nullptr )
    {
        getHead() = this;
    }
    else if( getHead()->m_Weight > m_Weight )
    {
        m_pNext = getHead();
        getHead() = this;
    }
    else
    {
        // We will do an insert short
        auto* pNext = getHead();
        for( ; pNext->m_pNext; pNext = pNext->m_pNext )
        {
            if( pNext->m_pNext->m_Weight > Weight )
            {
                m_pNext = pNext->m_pNext;
                pNext->m_pNext = this;
                break;
            }
        }

        // Added at the end of the list if we have added yet
        if( m_pNext == nullptr )
        {
            pNext->m_pNext = this;
        }
    }
}

//-------------------------------------------------------------------------------------------

type*& type::getHead( void ) noexcept
{ 
    static type* pHead{ nullptr }; 
    return pHead; 
}

//-------------------------------------------------------------------------------------------

base::base( xcore::string::constant<char> Str, main& MainDoc ) noexcept
    : xcore::system::registration{ *this, xcore::string::const_universal{ Str, xcore::string::constant<wchar_t>( L"Unnamed" ) }}
    , m_MainDoc{ MainDoc }
    , m_LogChannel{ Str.m_pValue }
{ 
    xcore::string::Copy(m_DocName, Str );
}

//-------------------------------------------------------------------------------------------

xcore::err base::onSaveProjectSettings( void )
{
/******
    xcore::vector< xproperty_v2::entry> List;

    //
    // Okay collect all properties
    //
    EnumProperty( [&]() -> xproperty_v2::entry&
    {
        return List.append();
    }, true, enum_mode::SAVE );

    // If there is not properties to save lets skip saving the settings for this editor
    if( List.getCount() == 0 ) return err::state::OK;

    //
    // Create the filename
    //
    xwstring  FileName;
    {
        xwstring  temp;
        temp = xstring( getType().m_TypeName ).To<xwchar>();
        x_strreplace<xwchar>( temp, L'/', L'-' );

        FileName.Copy( getMainDoc().getProjectSettingsPath().generic_wstring().c_str() );
        FileName.append(L"/");
        FileName.append( temp );
        FileName.append( L".txt");
    }

    //
    // Open file
    //
    xtextfile File;
    if( auto Error = File.openForWriting( FileName ); Error.isError() )
    {
        X_LOG_CHANNEL_INFO( m_LogChannel
            , "Error saving the settings for (%s) (error message: %s)"
            , static_cast<const char*>(FileName.To<xchar>())
            , Error.getString()
        );
        Error.clear();
        return { err::state::FAILURE, "Fail to save settings file" };
    }

    // Add the types for properties
    xproperty_v2::entry::RegisterTypes( File );

    //
    // Save all properties
    //
    File.WriteRecord("Properties", List.getCount<int>());

    for( const auto& Entry : List )
    {
        if (auto Err = Entry.SerializeOut(File); Err.isError())
            return Err.Forward<err>();
    }

    return err::state::OK;
    *****/
    return {};
}

//-------------------------------------------------------------------------------------------

xcore::err base::onLoadProjectSettings( void )
{
/**********
    //
    // Create the filename
    //
    xwstring  FileName;
    {
        xwstring  temp;
        temp = xstring( getType().m_TypeName ).To<xwchar>();
        x_strreplace<xwchar>( temp, L'/', L'-' );

        FileName.Copy( getMainDoc().getProjectSettingsPath().generic_wstring().c_str() );
        FileName.append(L"/");
        FileName.append( temp );
        FileName.append( L".txt");
    }

    //
    // Check if we have a setting file
    //
    std::error_code         ec;
    if( false == std_fs::exists( static_cast<const xwchar*>(FileName), ec ) )
        if( ec )
        {
            X_LOG_CHANNEL_INFO( m_LogChannel
                , "Error reading the settings for (%s) (error message: does file exits?, %s)"
                , static_cast<const char*>(FileName.To<xchar>())
                , ec.message().c_str()
            );
            return { err::state::FAILURE, "Fail to load settings file" };
        }
        else return err::state::OK;

    //
    // Open file
    //
    xtextfile File;
    if( auto Error = File.openForReading( FileName ); Error.isError() )
    {
        X_LOG_CHANNEL_INFO( m_LogChannel
            , "Error reading the settings for (%s) (error message: %s)"
            , static_cast<const char*>(FileName.To<xchar>())
            , Error.getString()
        );
        Error.clear();
        return { err::state::FAILURE, "Fail to load settings file" };
    }

    //
    // Load settings
    //
    if( auto Err = File.ReadRecord(); Err.isError() )
    {
        X_LOG_CHANNEL_INFO( m_LogChannel
            , "Error reading the settings for (%s) (error reading properties) (%s)"
            , Err.getStringAndClear()
            , static_cast<const char*>(FileName.To<xchar>())
        );
        return { err::state::FAILURE, "Fail to load settings file" };
    }

    if( auto Err = Load( File ); Err.isError() )
    {
        X_LOG_CHANNEL_INFO( m_LogChannel
                            , "Error reading the settings for (%s) (error reading properties) (%s)"
                            , Err.getStringAndClear()
                            , static_cast<const char*>( FileName.To<xchar>() )
        );
        return { err::state::FAILURE, "Fail to load settings file" };
    }

    return err::state::OK;
    ***********/

    return {};
}


//-------------------------------------------------------------------------------------------

xcore::err base::onLoadEditorSettings( void )
{
    return {};
}

//-------------------------------------------------------------------------------------------

xcore::err base::onSaveEditorSettings( void )
{
    return {};
}

//---------------------------------------------------------------------------------------------------
// END
//---------------------------------------------------------------------------------------------------
} // namespace xeditor::document