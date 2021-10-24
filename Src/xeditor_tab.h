namespace xeditor::tab
{
    using                   plugin_group_guid   = xcore::guid::group< struct xeditor_plugin_tag >;
    constexpr static auto   type_guid_v         = plugin_group_guid{ "xeditor::tab" };
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
                ,       m_bTabContainer:1
                ,       m_bMenuBar:1
                ,       m_bCustomBackgroundColor:1
                ,       m_bDisplayOnEmptyProject:1
                ;
            };
        };

                                                            type        ( xcore::string::constant<char> GlobalRegString
                                                                        , xcore::string::constant<char> Str
                                                                        , tab::type_guid                Guid
                                                                        , flags                         Flags           = {}
                                                                        , int                           Weight          = 0
                                                                        , xcore::icolor                 Col             = xcore::icolor{ ~0u } 
                                                                        );
        inline static           type*&                      getHead     ( void );
        virtual                 std::unique_ptr<tab::base>  New         ( xeditor::frame::base& EditorFrame)        const = 0;
        inline                  instance_guid               CreateInstanceGuid( void ) const;


        type*                                   m_pNext{ nullptr };
        const xcore::string::constant<char>     m_TypeName;
        tab::type_guid                          m_TypeGuid;
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

                                                base                ( xcore::string::constant<char>& Str, instance_guid Guid, xeditor::frame::base& EditorFrame );
        virtual                                ~base                ( void ) = default;
        virtual         const type&             getType             ( void ) = 0;
                        void                    Render              ( void );
        inline          bool                    isOpen              ( void ) const { return m_OpenPanel; }
        inline          void                    setOpen             ( bool bOpen ) { m_OpenPanel = bOpen; }
      //  inline          void                    setupDockSlot       ( ImGuiDockSlot DockSlot ) { m_DockSlot = DockSlot; }

        inline         xeditor::document::main& getMainDoc          ( void );
        inline          bool                    isVisible           ( void )                                            const   noexcept { return m_OpenPanel && m_bPanelVisible; }
        inline          void                    setActive           ( void )                                                    noexcept { m_bSetActive = true; }

    protected:

        virtual         void            onLogic                     ( void ) {}
        virtual         void            onRender                    ( void ) = 0;
        virtual         void            onCloseProject              ( void ) {}

    protected:

        xcore::cstring                  m_TabName;
        instance_guid                   m_InstanceGuid      { xcore::not_null };
        xeditor::frame::base&           m_EditorFrame;
        bool                            m_OpenPanel         { true };
  //      ImGuiDockSlot                   m_DockSlot          { ImGuiDockSlot_Tab };
        bool                            m_bPanelVisible     { true };
        bool                            m_bSetActive        { false };
        bool                            m_bFirstTimeRender  { true };

        friend class  xeditor::frame::base;
    };
}
