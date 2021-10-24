namespace xeditor::frame
{
    using                   plugin_group_guid   = xcore::guid::group< struct xeditor_plugin_tag >;
    constexpr static auto   type_guid_v         = plugin_group_guid{ "xeditor::frame" };
    using                   type_guid           = xcore::guid::subgroup<plugin_group_guid>;
    using                   instance_guid       = xcore::guid::unit<64, struct tab_instance_guid_tag >;

    //--------------------------------------------------------------------------------------

    struct type
        : xcore::property::base
        , xcore::system::registration
    {
        property_vtable();

        union flags
        {
            std::uint8_t m_Value = 0u;

            struct
            {
                bool    m_bDeleteWhenClose:1
                ,       m_bParentFrame:1
                ,       m_bMenuBar:1
                ,       m_bCustomBackgroundColor:1
                ,       m_bDisplayOnEmptyProject:1
                ;
            };
        };

                                                            type        ( xcore::string::constant<char> GlobalRegString
                                                                        , xcore::string::constant<char> Str
                                                                        , frame::type_guid              TypeGuid
                                                                        , frame::type_guid              ParentTypeGuid
                                                                        , flags                         Flags           = {}
                                                                        , int                           Weight          = 0
                                                                        , xcore::icolor                 Col             = xcore::icolor{ ~0u } 
                                                                        );
        inline static           type*&                       getHead     ( void );
        virtual                 std::unique_ptr<frame::base> New         ( xeditor::frame::main& EditorFrame)        const = 0;
        inline                  instance_guid                CreateInstanceGuid( void ) const;


        type*                                   m_pNext{ nullptr };
        const xcore::string::constant<char>     m_TypeName;
        frame::type_guid                        m_TypeGuid;
        frame::type_guid                        m_ParentTypeGuid;
        const flags                             m_Flags;
        const int                               m_Weight;
        xcore::icolor                           m_CustomBgColor;
        mutable instance_guid                   m_GuidSequence{0ull};
    };

    //--------------------------------------------------------------------------------------

    class base
    {
        xcore_rtti_start(base);

    public:

                                                base                ( xcore::string::constant<char>& Str, instance_guid Guid, xeditor::frame::main& EditorFrame );
        virtual                                ~base                ( void ) = default;
        virtual         const type&             getType             ( void ) = 0;
                        void                    Render              ( void );
        inline          bool                    isOpen              ( void ) const { return m_OpenPanel; }
        inline          void                    setOpen             ( bool bOpen ) { m_OpenPanel = bOpen; }
        inline         xeditor::document::main& getMainDoc          ( void );
        inline          bool                    isVisible           ( void )                                            const   noexcept { return m_OpenPanel && m_bPanelVisible; }
        inline          void                    setActive           ( void )                                                    noexcept { m_bSetActive = true; }
        inline          instance_guid           getGuid             ( void ) const noexcept { return m_InstanceGuid; }
        inline          const ImGuiWindowClass& getImGuiClass       ( void ) const noexcept { return m_ImGuiClass; }
    
    protected:

        virtual         void            onLogic                     ( void ) {}
        virtual         void            onRender                    ( void ) = 0;
        virtual         void            onCloseProject              ( void ) {}

    protected:

        xcore::cstring                  m_TabName;
        instance_guid                   m_InstanceGuid      { xcore::not_null };
        instance_guid                   m_ParentInstanceGuid{};
        xeditor::frame::main&           m_MainFrame;
        bool                            m_OpenPanel         { true };
        ImGuiWindowClass                m_ImGuiClass        {};
        const ImGuiWindowClass*         m_pParentImGuiClass {nullptr};
        bool                            m_bPanelVisible     { true };
        bool                            m_bSetActive        { false };
        bool                            m_bFirstTimeRender  { true };

        friend class  xeditor::frame::main;
    };
}
