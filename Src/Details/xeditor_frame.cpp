
#include "../dependencies/xGPU/Tools/xgpu_imgui_breach.cpp"

namespace xeditor::frame {

//-------------------------------------------------------------------------------------------
base::~base( void )
{
    //
    // Shut down
    //
    xgpu::tools::imgui::Shutdown();
}

//-------------------------------------------------------------------------------------------

void base::_SystemMainInit( xeditor::document::main& MainDoc )
{
    m_pDocument = &MainDoc;
    m_delegatemsgCloseProject.Connect( MainDoc.m_Events.m_CloseProject );

    for( auto* pNext = xeditor::tab::type::getHead(); pNext; pNext = pNext->m_pNext )
    {
        pNext->RegisterSystem();
    }
}

//-------------------------------------------------------------------------------------------

base::base( xgpu::window& Window )
{ 
    //
    // Save the window
    //
    m_XGPUWindow = Window;
    m_XGPUWindow.getDevice(m_XGPUDevice);
    m_XGPUDevice.getInstance(m_XGPUInstance);

    //
    // Initialize the view
    //
    m_View.setFov           ( 91.5_xdeg );
    m_View.setFarZ          ( 256.0f );
    m_View.setNearZ         ( 0.1f );
    m_View.LookAt           ( 2.5f, xcore::radian3( 0_xdeg, 0_xdeg, 0_xdeg ), xcore::vector3(0) );

    // User Initialize
    onSetup(); 
}

//-------------------------------------------------------------------------------------------

bool base::AdvanceLogic( void )
{ 
    return onAdvanceLogic(); 
}

//-------------------------------------------------------------------------------------------

void base::onSetup( void )
{
    InitializeImGui();
}

//-------------------------------------------------------------------------------------------

void base::InitializeImGui( void )
{
    if( auto Err = xgpu::tools::imgui::CreateInstance( m_XGPUWindow ); Err )
        throw(std::runtime_error( xgpu::getErrorMsg(Err) ));

    //
    // Add fonts
    //
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // merge in icons from Font Awesome
    {
        static constexpr auto icons_ranges_fontawesome     = std::array{ ImWchar{ICON_MIN_FA}, ImWchar{ICON_MAX_FA}, ImWchar{0} };
        static constexpr auto icons_ranges_kenney          = std::array{ ImWchar{ICON_MIN_KI}, ImWchar{ICON_MAX_KI}, ImWchar{0} };
        static constexpr auto icons_ranges_materialdesign  = std::array{ ImWchar{ICON_MIN_MD}, ImWchar{ICON_MAX_MD}, ImWchar{0} };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF( "fontawesome-webfont.ttf", 14.0f, &icons_config, icons_ranges_fontawesome.data());
        //io.Fonts->AddFontFromFileTTF( "../kenney-icon-font.ttf",    16.0f, &icons_config, icons_ranges_kenney);
        //  io.Fonts->AddFontFromFileTTF( "../MaterialIcons-Regular.ttf", 13.0f, &icons_config, icons_ranges_materialdesign);
    }

    //
    // Set the log function
    //
    //g_context::get().m_pLogOuputFn = MyLogOutput;
}

//-------------------------------------------------------------------------------------------

void base::onBegingRender( void )
{
//    m_Window->BeginRender( m_View );
}

//-------------------------------------------------------------------------------------------
void base::onEndRender( void )
{
    m_XGPUWindow.PageFlip();
}

//-------------------------------------------------------------------------------------------

bool base::BeginFrame( void )
{
    //
    // Check if we need to render
    //
    if( xgpu::tools::imgui::BeginRendering(true) )
        return true;

    //
    // Make the camera look at the center of the screen
    // Rotate slowly
    //
    m_View.setViewport( xcore::irect{ 0, 0, m_XGPUWindow.getWidth(), m_XGPUWindow.getHeight() } );

    /*
    auto CameraAngles   = m_View.getAngles(); 
    CameraAngles.m_Yaw += xradian{ 1_deg };
    m_View.LookAt( 4.5, CameraAngles, xvector3(0) );
    */

    //
    // We are ready to beging rendering
    //
    onBegingRender();

    /***********************
    //
    // Get imGuid ready for action
    //
    //ImGui_engBase_NewFrame();

    //
    // Need to create the maindoc window. This window will be consider our main application window.
    // We need this because the docking system need a parent window to dock. So we will keep this
    // maindoc window as big as the system window at all times.
    // Since it is a fake window in a way we want to make sure that we dont render any background
    // So we will make its background 100% transparent so we can see perfectly thought it.
    // The docking also space also tries not render a the background so we will also make it 100%
    // transparent.
    //
    const ImGuiStyle *  style               = &ImGui::GetStyle();
    const ImColor       TempBgColor         = style->Colors[ImGuiCol_WindowBg];
    const ImColor       TempChildBgColor    = style->Colors[ImGuiCol_ChildBg];

    // Make sure that our main doc is as big as the system window 
    ImGui::SetNextWindowPos( ImVec2(0,0) );
    ImGui::SetWindowSize( "maindoc", ImVec2( (float)m_View.getViewport().getWidth(), (float)View.getViewport().getHeight()), 0 );

    // make 100% transparent
    static const ImVec4 col{0,0,0,0};
    ImGui::PushStyleColor(ImGuiCol_WindowBg, col);
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, col);

    // Now we can render the window which will take the hold screen
    static bool bOPen = false;
    ImGui::Begin( "maindoc", &bOPen, 
        ImGuiSetCond_Always                     |
        ImGuiWindowFlags_NoMove                 |
        ImGuiWindowFlags_NoResize               |
        ImGuiWindowFlags_NoInputs               |
        ImGuiWindowFlags_NoTitleBar             |
        ImGuiWindowFlags_NoSavedSettings        |
        ImGuiWindowFlags_NoScrollbar            |
        ImGuiWindowFlags_NoCollapse             |
  //      ImGuiWindowFlags_MenuBar                |
        ImGuiWindowFlags_NoFocusOnAppearing     |
        ImGuiWindowFlags_AlwaysUseWindowPadding |
        ImGuiWindowFlags_NoBringToFrontOnFocus );

    // Start the doc space now  
    ImGui::BeginDockspace();

    // Restore the original colors of the theme
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, TempChildBgColor);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, TempBgColor );
    ***********************************/

    return false;
}

//-------------------------------------------------------------------------------------------
void base::EndFrame( void )
{
    //
    // close the imgGui window
    //

/*
    // Pop the orifinal colors of the theme
    ImGui::PopStyleColor(2); 

    // End the maindoc window    
    ImGui::EndDockspace();
    ImGui::End();
*/
    // Now we can pop the other two transparent colors (child and window)
    ImGui::PopStyleColor(2); 

    //
    // Draw the gui
    //
    ImGui::Render();

    //
    // Tell the engine we are done rendering
    //
    onEndRender();
}

//-------------------------------------------------------------------------------------------

xcore::err base::CreateTab( const char* pString, /*ImGuiDockSlot Slot, */bool bActive) noexcept
{
/*
    for( auto* pNext = xeditor::plugins::tab::type::getHead(); pNext; pNext = pNext->m_pNext )
    {
        if ( x_strstr<xchar>( pNext->m_TypeName.m_pValue, pString ) >= 0 )
        {
            auto& Entry = m_lTab.append( pNext->New( *this ) );
            Entry->setupDockSlot( Slot );
            if( bActive ) 
            {
                Entry->setActive();
            }
            return err::state::OK;
        }
    }
*/
    return xerr_failure_s( "Count not find the tab" );
}

//-------------------------------------------------------------------------------------------

void base::onRenderWindows( void )
{
    //
    // Any mouse press should wake up the app
    //
    if( false == xgpu::tools::imgui::isInputSleeping() )
    {
        Awake();
    }

    //
    // Tell the system if I am sleeping 
    //
    //m_XGPUWindow->setPlayMode(m_CoolDown != 0);
    if (m_CoolDown)--m_CoolDown;

    //
    // Panels
    //
    for( int i=0; i<m_lTab.size(); i++ )
    {
        auto& xpTab = m_lTab[i];
        
        // Reset if the panel is visible or not
        xpTab->m_bPanelVisible = false;

        if( xpTab->isOpen() )
        {
            xpTab->Render();
        }
    }
}

//-------------------------------------------------------------------------------------------
void base::onCloseProject( void )
{
    //
    // Panels
    //
    for( int i=0; i<m_lTab.size(); i++ )
    {
        auto& xpTab = m_lTab[i];
        xpTab->onCloseProject();
    }
}

//-------------------------------------------------------------------------------------------
bool base::onAdvanceLogic( void )
{
    //
    // Close any tabs that need to be close first
    //
    for( auto& Entry : m_DelayCmds )
    {
        Entry();
    }
    m_DelayCmds.clear();

    //
    // Close any tabs that need to be close first
    //
    for( int i=0; i<m_lTab.size(); i++ )
    {
        auto& xpTab = m_lTab[i];
        if( xpTab->getType().m_Flags.m_bDeleteWhenClose )
        {
            m_lTab.eraseCollapse(i);
            --i;
        }
    }

    //
    // Read all the input events
    //
    if (m_XGPUInstance.ProcessInputEvents())
        return false;

    //
    // Run logic before trying to render anything
    //
    for( int i=0; i<m_lTab.size(); i++ )
    {
        auto& xpTab = m_lTab[i];

        if( xpTab->isOpen() )
        {
            xpTab->onLogic();
        }
    }

    //
    // Now we can render the windows
    //
    if( BeginFrame() )
        return true;

    // Render windows
    onRenderWindows();

    // Done with the frame
    EndFrame();

    //
    // Let the system know that we are at the end of the frame
    //
    getMainDoc().m_Events.m_EndOfFrame.NotifyAll();
    return true;
}

//---------------------------------------------------------------------------------------------------
// END
//---------------------------------------------------------------------------------------------------
} // namespace xeditor::frame