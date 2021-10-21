namespace xeditor::plugins
{
    using                   group_guid  = xcore::guid::group< struct xeditor_plugin_tag >;
    constexpr static auto   type_guid_v = group_guid{ "base::plugin" };
    using                   guid        = xcore::guid::subgroup<group_guid>;

    class tab
    {
        xcore_rtti_start(tab);

    public:

        struct type
            : xcore::property::base
            , protected xcore::system::registration
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

                                                                type                ( xcore::string::constant<char> GlobalRegString, xcore::string::constant<char> Str, flags Flags = {}, int Weight = 0, xcore::icolor Col = xcore::icolor{ ~0u } );
            static                  type*&                      getHead             ( void );
                                    guid                        getGuid             ( void )                                    const   noexcept;
            virtual                 xcore::unique_span<tab>     New                 ( xeditor::frame::base& EditorFrame)        const = 0;


            type*                                   m_pNext{ nullptr };
            const xcore::string::constant<char>     m_TypeName;
            const flags                             m_Flags;
            const int                               m_Weight;
            xcore::icolor                           m_CustomBgColor;
        };

        template< typename T_TYPE >
        struct type_harness : type
        {
            constexpr            type_harness(const xcore::string::constant<char> Str, flags Flags = {}, int Weight = 0, xcore::icolor Col = xcore::icolor{ ~0u } ) noexcept
                : type
                (
                    []( const xcore::string::constant<char> S ) noexcept
                    {
                        static std::array<char, 256> X;
                        int i = 0, j = 0;
                        while (X[i] = "$gbEditor/Tabs/"[i]) i++;
                        while (X[i + j] = S.m_pValue[j]) j++;
                        return xcore::string::constant<char>{ &X[1] };
                    }(Str)
                    , Str
                    , Flags
                    , Weight
                    , Col
                ) {}

            virtual xcore::unique_span<tab>   New( xeditor::frame::base& EditorFrame ) const override
            {
                return new T_TYPE{ m_TypeName, EditorFrame };
            }
        };

    public:

                                                tab                 ( const xcore::string::constant<char>& Str, xeditor::frame::base& EditorFrame );
        virtual                                ~tab                 ( void );
        virtual         const type&             getType             ( void ) = 0;
                        void                    Render              ( void );
        inline          bool                    isOpen              ( void ) const { return m_OpenPanel; }
        inline          void                    setOpen             ( bool bOpen ) { m_OpenPanel = bOpen; }
      //  inline          void                    setupDockSlot       ( ImGuiDockSlot DockSlot ) { m_DockSlot = DockSlot; }

        inline         xeditor::document::main& getMainDoc          ( void );// { return m_EditorFrame.getMainDoc<T>(); }
        inline          bool                    isVisible           ( void )                                            const   noexcept { return m_OpenPanel && m_bPanelVisible; }
        inline          void                    setActive           ( void )                                                    noexcept { m_bSetActive = true; }

    protected:

        virtual         void            onLogic                     ( void ) {}
        virtual         void            onRender                    ( void ) = 0;
        virtual         void            onCloseProject              ( void ) {}

    protected:

        xcore::cstring                  m_TabName;
        xeditor::frame::base&           m_EditorFrame;
        bool                            m_OpenPanel         { true };
  //      ImGuiDockSlot                   m_DockSlot          { ImGuiDockSlot_Tab };
        bool                            m_bPanelVisible     { true };
        bool                            m_bSetActive        { false };
        bool                            m_bFirstTimeRender  { true };

        friend class  xeditor::frame::base;
    };
}
