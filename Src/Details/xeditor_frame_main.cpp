
#include "../dependencies/xGPU/Tools/xgpu_imgui_breach.cpp"

namespace xeditor::frame {

//-------------------------------------------------------------------------------------------
main::~main( void )
{
    //
    // Shut down
    //
    xgpu::tools::imgui::Shutdown();
}

//-------------------------------------------------------------------------------------------

void main::_SystemMainInit( xeditor::document::main& MainDoc )
{
    m_pDocument = &MainDoc;
    m_delegatemsgCloseProject.Connect( MainDoc.m_Events.m_CloseProject );

    for( auto* pNext = xeditor::frame::type::getHead(); pNext; pNext = pNext->m_pNext )
    {
        pNext->RegisterSystem();
    }
}

//-------------------------------------------------------------------------------------------

main::main( xgpu::window& Window )
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

bool main::AdvanceLogic( void )
{ 
    return onAdvanceLogic(); 
}

//-------------------------------------------------------------------------------------------

void main::onSetup( void )
{
    InitializeImGui();
}

//-------------------------------------------------------------------------------------------

void main::InitializeImGui( void )
{
    //
    // Create IMGUI context outside the breach since we are going to be setting some fonts
    //
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    //
    // Add fonts
    //
    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();
    io.Fonts->AddFontDefault();

    // merge in icons from Font Awesome
    {
        static constexpr auto icons_ranges_fontawesome     = std::array{ ImWchar{ICON_MIN_FA}, ImWchar{ICON_MAX_FA}, ImWchar{0} };
        static constexpr auto icons_ranges_kenney          = std::array{ ImWchar{ICON_MIN_KI}, ImWchar{ICON_MAX_KI}, ImWchar{0} };
        static constexpr auto icons_ranges_materialdesign  = std::array{ ImWchar{ICON_MIN_MD}, ImWchar{ICON_MAX_MD}, ImWchar{0} };
        ImFontConfig icons_config{};
        icons_config.MergeMode = true;
        icons_config.GlyphOffset.y = 2;

        io.Fonts->AddFontFromMemoryCompressedTTF
        ( fa_solid_900_compressed_data
        , fa_solid_900_compressed_size
        , 14.0f
        , &icons_config
        , icons_ranges_fontawesome.data()
        );
       
        io.Fonts->AddFontFromMemoryCompressedTTF
        ( kenney_icon_font_compressed_data
        , kenney_icon_font_compressed_size
        , 14.0f
        , &icons_config
        , icons_ranges_kenney.data()
        );

        io.Fonts->AddFontFromMemoryCompressedTTF
        ( MaterialIcons_Regular_compressed_data
        , MaterialIcons_Regular_compressed_size
        , 13.0f
        , &icons_config
        , icons_ranges_materialdesign.data()
        );
    }

    //
    // Build the font
    //
    io.Fonts->Build();

    //
    // Enable docking
    //
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    //
    // Initialize the breach officially 
    //
    if (auto Err = xgpu::tools::imgui::CreateInstance(m_XGPUWindow); Err)
        throw(std::runtime_error(xgpu::getErrorMsg(Err)));
}

//-------------------------------------------------------------------------------------------

void main::onBegingRender( void )
{
//    m_Window->BeginRender( m_View );
}

//-------------------------------------------------------------------------------------------
void main::onEndRender( void )
{
    m_XGPUWindow.PageFlip();
}

//-------------------------------------------------------------------------------------------

bool main::BeginFrame( void )
{
    //
    // Check if we need to render
    //
    if( xgpu::tools::imgui::BeginRendering(false) )
        return true;

    //
    // Enable docking
    //
    {
        constexpr bool                  opt_padding = false;
        constexpr ImGuiDockNodeFlags    dockspace_flags = ImGuiDockNodeFlags_AutoHideTabBar
            | ImGuiDockNodeFlags_PassthruCentralNode;
        constexpr ImGuiWindowFlags      window_flags = ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Main DockSpace", nullptr, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        ImGui::PopStyleVar(2);

        //
        // Set the class ID for the main frame so that we can decide who can dock with us
        //
        m_ImGuiClass.ClassId = ImGui::GetID("MainFrame");

        static ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, &m_ImGuiClass );
        ImGui::End();
    }

    //
    // Make the camera look at the center of the screen
    // Rotate slowly
    //
    m_View.setViewport( xcore::irect{ 0, 0, m_XGPUWindow.getWidth(), m_XGPUWindow.getHeight() } );

    //
    // We are ready to beging rendering
    //
    onBegingRender();

    return false;
}

//-------------------------------------------------------------------------------------------
void main::EndFrame( void )
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
    //ImGui::PopStyleColor(2); 

    //
    // Draw the gui
    //
    xgpu::tools::imgui::Render();

    //
    // Tell the engine we are done rendering
    //
    onEndRender();
}

//-------------------------------------------------------------------------------------------

xcore::err main::CreateTab( const char* pString, /*ImGuiDockSlot Slot, */bool bActive) noexcept
{

    for( auto* pNext = xeditor::frame::type::getHead(); pNext; pNext = pNext->m_pNext )
    {
        if ( xcore::string::FindStr( pNext->m_TypeName.m_pValue, pString ) >= 0 )
        {
            auto& Entry = m_lFrames.append( pNext->New( *this ) );
            
            //Entry->setupDockSlot( Slot );
            if( bActive ) 
            {
                Entry->setActive();
            }
            return {};
        }
    }

    return xerr_failure_s( "Count not find the tab" );
}

//-------------------------------------------------------------------------------------------

void main::onRenderWindows( void )
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
    for( int i=0; i<m_lFrames.size(); i++ )
    {
        auto& xpTab = m_lFrames[i];
        
        // Reset if the panel is visible or not
        xpTab->m_bPanelVisible = false;

        if( xpTab->isOpen() )
        {
            xpTab->Render();
        }
    }
}

//-------------------------------------------------------------------------------------------
void main::onCloseProject( void )
{
    //
    // Panels
    //
    for( int i=0; i<m_lFrames.size(); i++ )
    {
        auto& xpTab = m_lFrames[i];
        xpTab->onCloseProject();
    }
}

//-------------------------------------------------------------------------------------------
bool main::onAdvanceLogic( void )
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
    for( int i=0; i<m_lFrames.size(); i++ )
    {
        auto& xpTab = m_lFrames[i];
        if( xpTab->getType().m_Flags.m_bDeleteWhenClose )
        {
            m_lFrames.eraseCollapse(i);
            --i;
        }
    }

    //
    // Read all the input events
    //
    if( false == m_XGPUInstance.ProcessInputEvents() )
        return false;

    //
    // Run logic before trying to render anything
    //
    for( int i=0; i<m_lFrames.size(); i++ )
    {
        auto& xpTab = m_lFrames[i];

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


    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);


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