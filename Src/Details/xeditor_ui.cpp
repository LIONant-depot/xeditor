namespace xeditor::ui {

//-------------------------------------------------------------------------------------------

void ToolTip( const char* pfmt )
{
    if ( ImGui::IsItemHovered() )
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos( 450.0f );
        ImGui::TextUnformatted( pfmt );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//-------------------------------------------------------------------------------------------

void MenuToolTip( const char* pfmt )
{
    ImGui::SameLine( 200 );
    ImGui::TextDisabled( ICON_FA_QUESTION_CIRCLE );
    ToolTip( pfmt );
}

//-------------------------------------------------------------------------------------------

void MenuInfo( const char* pfmt )
{
    ImGui::Text( ICON_FA_INFO_CIRCLE );
    ToolTip( pfmt );
    ImGui::SameLine( 20 );
}

//-------------------------------------------------------------------------------------------

void MenuSameLineAfterIcon ( void )
{
    ImGui::SameLine( 30 );
}

//-------------------------------------------------------------------------------------------

bool MenuSwitchButton( bool bState, const char* pButtonName, const char* pHelp )
{
    ImGui::PushID( pButtonName );
    if ( ImGui::Button( bState ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF ) ) { bState = !bState; }
    ImGui::PopID();
    MenuSameLineAfterIcon(); ImGui::Text( pButtonName );
    if ( pHelp ) MenuToolTip( pHelp );
    return bState;
}

//-------------------------------------------------------------------------------------------

void details::CircleButtonStylingPush( void )
{
    ImGui::PushStyleColor( ImGuiCol_Button,         ImVec4( 0.0f, 0.5f, 0.2f, 1.0f) );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered,  ImVec4( 0.0f, 0.7f, 0.2f, 1.0f) );
    ImGui::PushStyleColor( ImGuiCol_ButtonActive,   ImVec4( 0.0f, 0.8f, 0.2f, 1.0f) );
}

//-------------------------------------------------------------------------------------------

void details::CircleButtonStylingPop( void )
{
    ImGui::PopStyleColor( 3 );
}

//-------------------------------------------------------------------------------------------

bool CircleButton( void )
{
    bool bPressed = false;
    details::CircleButtonStylingPush();
    if ( ImGui::Button( " " ICON_FA_ARROW_CIRCLE_DOWN " " ) )
    {
        bPressed = true;
    }
    details::CircleButtonStylingPop();
    return bPressed;
}

//-------------------------------------------------------------------------------------------
bool MenuItem( const char* pName, const char* pHelp )
{
    const bool bAnswer = ImGui::MenuItem( pName );
    MenuToolTip( pHelp );
    return bAnswer;
}

//-------------------------------------------------------------------------------------------

void Separator( void )
{
    ImGui::Separator();
}

//-------------------------------------------------------------------------------------------

void name_dlg::operator() ( const char* pName, std::span<char>  Buffer )
{
    if ( m_State == state::OK || m_State == state::CANCEL )
    {
        m_State = state::NOT_OPEN;
    }

    if ( m_State == state::NOT_OPEN )
    {
        ImGui::OpenPopup( pName );
    }

    m_State = state::RUNNING;

    if ( ImGui::BeginPopupModal( pName ) )
    {
        ImGui::Text( "Name: " );
        ImGui::SameLine(); ImGui::InputText( "", Buffer.data(), Buffer.size() );

        if ( ImGui::Button( "OK", ImVec2( 120, 0 ) ) )
        {
            m_State = state::OK;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if ( ImGui::Button( "Cancel", ImVec2( 120, 0 ) ) )
        {
            m_State = state::CANCEL;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

//---------------------------------------------------------------------------------------------------
// END
//---------------------------------------------------------------------------------------------------
} // namespace xeditor::ui