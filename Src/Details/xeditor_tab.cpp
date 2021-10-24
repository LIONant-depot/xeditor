//-------------------------------------------------------------------------------------------
//  Properties for the base doc must be in a global scope
//-------------------------------------------------------------------------------------------
property_begin_name(xeditor::tab::type, "BaseTab")
{
    property_var_fnbegin("Name", xcore::cstring)
    {
        if (isRead) xcore::string::Copy(InOut, Self.m_TypeName );
    } property_var_fnend()
        .Flags(property::flags::SHOW_READONLY | property::flags::DONTSAVE)
        .Help("Name of the factory that generates Editor tabs for a particular type of tab")

} property_vend_cpp(xeditor::tab::type)

//-------------------------------------------------------------------------------------------
// Back to the expected CPP
//-------------------------------------------------------------------------------------------
namespace xeditor::tab {

//-------------------------------------------------------------------------------------------

type::type
( xcore::string::constant<char> GlobalRegString
, xcore::string::constant<char> Str
, tab::type_guid                     Guid
, type::flags                   Flags
, int                           Weight
, xcore::icolor                 Color 
) 
: xcore::property::base()
, xcore::system::registration( nullptr, *this, xcore::string::const_universal{ GlobalRegString, L"Unnamed" } )
, m_TypeName        { Str       }
, m_TypeGuid            { Guid      }
, m_Flags           { Flags     }
, m_Weight          { Weight    }
, m_CustomBgColor   { Color     }
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

base::base( xcore::string::constant<char>& Str, instance_guid Guid, xeditor::frame::base& EditorFrame )
: m_EditorFrame { EditorFrame }
, m_InstanceGuid{ Guid }
{
    m_TabName = xcore::string::Fmt("%s##%u", Str.m_pValue, Guid.m_Value );
}

//-------------------------------------------------------------------------------------------

void base::Render( void )
{
    const auto bEmptyProject = getMainDoc().isProjectEmpty();

    ///////////////ImGui::SetNextDock( m_DockSlot );

    auto& Type = getType();

    if( Type.m_Flags.m_bCustomBackgroundColor ) 
    {
        auto V4 = Type.m_CustomBgColor.getRGBA();
        ImVec4 Col{ V4.m_X, V4.m_Y, V4.m_Z, V4.m_W };
        ImGui::PushStyleColor( ImGuiCol_ChildBg, Col );
    }

    if ( m_bPanelVisible = ImGui::Begin( m_TabName, &m_OpenPanel, Type.m_Flags.m_bMenuBar?ImGuiWindowFlags_MenuBar:0 ) ) 
    {
        if( Type.m_Flags.m_bTabContainer )
        {
             ////////////////ImGui::BeginDockspace();
        }

        if( bEmptyProject == false )
        {
            onRender();
        }
        else
        {
            if( Type.m_Flags.m_bDisplayOnEmptyProject ) 
            {
                onRender();
            }
        }

        if( Type.m_Flags.m_bTabContainer )
        {
            //////////////ImGui::EndDockspace();
        }


        ImGui::End();
    }
    
    if( m_bSetActive )
    {
        if(false == m_bFirstTimeRender) m_bSetActive = false;
        ////////////ImGui::SetDockActive();
    }

    //////////ImGui::EndDock();

    if( Type.m_Flags.m_bCustomBackgroundColor ) 
    {
        ImGui::PopStyleColor();
    }

    // No longer the first time rendering
    m_bFirstTimeRender = false;
}


//----------------------------------------------------------------------------------------
// END
//----------------------------------------------------------------------------------------
} // namespace xeditor::tab
