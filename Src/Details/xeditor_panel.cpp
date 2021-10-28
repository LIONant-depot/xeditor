//-------------------------------------------------------------------------------------------
//  Properties for the base doc must be in a global scope
//-------------------------------------------------------------------------------------------
property_begin_name(xeditor::panel::type, "BaseTab")
{
    property_var_fnbegin("Name", xcore::cstring)
    {
        if (isRead) xcore::string::Copy(InOut, Self.m_TypeName );
    } property_var_fnend()
        .Flags(property::flags::SHOW_READONLY | property::flags::DONTSAVE)
        .Help("Name of the factory that generates Editor tabs for a particular type of tab")

} property_vend_cpp(xeditor::panel::type)

//-------------------------------------------------------------------------------------------
// Back to the expected CPP
//-------------------------------------------------------------------------------------------
namespace xeditor::panel {

//-------------------------------------------------------------------------------------------

type::type
( xcore::string::constant<char> GlobalRegString
, xcore::string::constant<char> Str
, panel::type_guid              TypeGuid
, type::flags                   Flags
, int                           Weight
, xcore::icolor                 Color 
) 
: xcore::property::base()
, xcore::system::registration( nullptr, *this, xcore::string::const_universal{ GlobalRegString, L"Unnamed" } )
, m_TypeName        { Str       }
, m_TypeGuid        { TypeGuid  }
, m_Flags           { Flags     }
, m_Weight          { Weight    }
, m_CustomBgColor   { Color     }
{ 

    type** pHead = &getHead();

    if( *pHead == nullptr )
    {
        *pHead = this;
    }
    else if( getHead()->m_Weight > m_Weight )
    {
        m_pNext = *pHead;
        *pHead = this;
    }
    else
    {
        // We will do an insert short
        auto* pNext = *pHead;
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

base::base( construct& Construct )
: m_Type { Construct.m_Type }
, m_InstanceGuid{ Construct.m_Guid }
, m_TabName{ xcore::string::Fmt("%s##%u", Construct.m_Str.m_pValue, Construct.m_Guid.m_Value)  }
{
}

//-------------------------------------------------------------------------------------------
child::child( construct& Construct )
: base(Construct)
, m_Parent{ Construct.m_Parent }
{
}

//-------------------------------------------------------------------------------------------

parent::parent( construct& Construct )
: base( Construct )
, m_AppFrame{ Construct.m_EditorFrame }
{
}

//-------------------------------------------------------------------------------------------

xcore::err parent::CreateTab(const char* pString, bool bActive) noexcept
{
    for (auto* pNext = xeditor::panel::type::getHead(); pNext; pNext = pNext->m_pNext)
    {
        if (m_Type.m_TypeGuid == pNext->m_TypeGuid)
        {
            for (auto* p = pNext->m_pChildren; p; p = p->m_pNext)
            {
                if (xcore::string::FindStr( p->m_TypeName.m_pValue, pString) >= 0)
                {
                    auto& Entry = m_lChildPanels.append(p->New(*this));
                    if (bActive)
                    {
                        Entry->setActive();
                    }
                    return {};
                }
            }
        }
    }

    return xerr_failure_s("Count not find the child tab");
}

//-------------------------------------------------------------------------------------------

void parent::Render( void )
{
    const auto bEmptyProject = getMainDoc().isProjectEmpty();

    ///////////////ImGui::SetNextDock( m_DockSlot );

    if(m_Type.m_Flags.m_bCustomBackgroundColor )
    {
        auto V4 = m_Type.m_CustomBgColor.getRGBA();
        ImVec4 Col{ V4.m_X, V4.m_Y, V4.m_Z, V4.m_W };
        ImGui::PushStyleColor( ImGuiCol_ChildBg, Col );
    }

    if( m_pParentImGuiClass == nullptr )
    {
        auto ParentGuid = m_Type.m_ParentTypeGuid;
        m_pParentImGuiClass = &m_AppFrame.getImGuiClass();

        // Set our own ID as well... (if we have to)
        m_ImGuiClass.DockingAllowUnclassed = false;
        m_ImGuiClass.ClassId = ImGui::GetID(m_TabName);
    }

    ImGui::SetNextWindowClass(m_pParentImGuiClass);
    if ( m_bPanelVisible = ImGui::Begin( m_TabName, &m_OpenPanel, m_Type.m_Flags.m_bMenuBar?ImGuiWindowFlags_MenuBar:0 ) )
    {
        if( m_Type.m_Flags.m_bDisableChildDocking == false )
        {
            ImGui::DockSpace(m_ImGuiClass.ClassId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoDockingInCentralNode 
                                                                     | ImGuiDockNodeFlags_AutoHideTabBar
                                                                     , &m_ImGuiClass );
        }

        if( bEmptyProject == false )
        {
            onRender();
        }
        else
        {
            if(m_Type.m_Flags.m_bDisplayOnEmptyProject )
            {
                onRender();
            }
        }

        for( auto& E : m_lChildPanels ) E->Render();
    }
    ImGui::End();

    if( m_bSetActive )
    {
        if(false == m_bFirstTimeRender) m_bSetActive = false;
        //ImGui::SetActive();
    }

    if(m_Type.m_Flags.m_bCustomBackgroundColor )
    {
        ImGui::PopStyleColor();
    }

    // No longer the first time rendering
    m_bFirstTimeRender = false;
}

//-------------------------------------------------------------------------------------------

void child::Render(void)
{
    ImGui::SetNextWindowClass( &m_Parent.getImGuiClass() );
    if (m_bPanelVisible = ImGui::Begin(m_TabName, &m_OpenPanel, m_Type.m_Flags.m_bMenuBar ? ImGuiWindowFlags_MenuBar : 0))
    {
        onRender();
    }
    ImGui::End();

    // No longer the first time rendering
    m_bFirstTimeRender = false;
}

//----------------------------------------------------------------------------------------
// END
//----------------------------------------------------------------------------------------
} // namespace xeditor::tab
