namespace xeditor::frame
{
    class base
    {
    public:

                                                base                ( void ) = default;
                                                base                ( xgpu::window& Window );
        virtual                                ~base                ( void );
//                        base&                   setupEngWindow      ( xgpu::window& Window );
                        bool                    AdvanceLogic        ( void );
        inline         xeditor::document::main& getMainDoc          ( void )                                                                                    noexcept;
                        xcore::err              CreateTab           ( const char* pString, bool bActive = false )                                               noexcept;
        inline          auto&                   getTabList          ( void )                                                                                    noexcept;
        xforceinline    xgpu::window&           getWindow           ( void )                                                                                    noexcept;
        xforceinline    void                    Awake               ( void )                                                                                    noexcept;
        
        template< typename T > 
        xforceinline    void                    appendDelayCmd      ( T&& Function )                                                                            noexcept;

    protected:

        virtual         void                    onSetup             ( void );
        virtual         void                    onRenderWindows     ( void );
        virtual         bool                    onAdvanceLogic      ( void );
        virtual         void                    onBegingRender      ( void );
        virtual         void                    onEndRender         ( void );
        virtual         void                    onAwake             ( void );
                        void                    InitializeImGui     ( void );
                        bool                    BeginFrame          ( void );
                        void                    EndFrame            ( void );
        virtual         void                    onCloseProject      ( void );

    protected:

        xeditor::document::main*                                    m_pDocument                 { nullptr };
        xgpu::window                                                m_XGPUWindow                {};
        xgpu::device                                                m_XGPUDevice                {};
        xgpu::instance                                              m_XGPUInstance              {};
        xgpu::device                                                m_XGPUMouse                 {};
        xgpu::device                                                m_XGPUKeyboard              {};
        xgpu::tools::view                                           m_View                      {};
        xcore::vector<std::unique_ptr<xeditor::tab::base>>          m_lTab                      {}; 
        int                                                         m_CoolDown                  {10};
        document::main::events::close_project::delegate             m_delegatemsgCloseProject   { this, &base::onCloseProject };
        xcore::vector<xcore::func<void(void)>>                      m_DelayCmds                 {};

    public:

                        void                    _SystemMainInit      ( xeditor::document::main& MainDoc );
    };
}