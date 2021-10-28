namespace xeditor::panel
{
    using                   plugin_group_guid   = xcore::guid::group< struct xeditor_plugin_tag >;
    constexpr static auto   type_guid_v         = plugin_group_guid{ "xeditor::panel" };
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
                ,       m_bMenuBar:1
                ,       m_bCustomBackgroundColor:1
                ,       m_bDisplayOnEmptyProject:1
                ,       m_bDisableChildDocking :1
                ;
            };
        };

                                                            type        ( xcore::string::constant<char> GlobalRegString
                                                                        , xcore::string::constant<char> Str
                                                                        , panel::type_guid              TypeGuid
                                                                        , flags                         Flags           = {}
                                                                        , int                           Weight          = 0
                                                                        , xcore::icolor                 Col             = xcore::icolor{ ~0u } 
                                                                        );

        inline static           type*&                          getHead     ( void );
        inline static           type*&                          getChildren ( void );
        virtual                 std::unique_ptr<panel::parent>  New         ( xeditor::frame& EditorFrame)          const = 0;
        virtual                 std::unique_ptr<panel::child>   New         ( panel::parent& Parent )               const = 0;
        inline                  instance_guid                   CreateInstanceGuid( void ) const;


        type*                                   m_pNext{ nullptr };
        type*                                   m_pChildren{ nullptr };
        const xcore::string::constant<char>     m_TypeName;
        panel::type_guid                        m_TypeGuid;
        panel::type_guid                        m_ParentTypeGuid;
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

        struct construct
        {
            const xcore::string::constant<char>&    m_Str;
            instance_guid                           m_Guid;
            const type&                             m_Type;
        };

    public:

        virtual                                ~base                ( void ) = default;
                                                base                ( construct& Construct );
        inline          bool                    isVisible           ( void )                                            const   noexcept;
        inline          void                    setActive           ( void )                                                    noexcept;
        inline          instance_guid           getGuid             ( void ) const noexcept;
        inline          bool                    isOpen              ( void ) const noexcept;
        inline          void                    setOpen             ( bool bOpen );

    protected:

        const type&                             m_Type;
        xcore::cstring                          m_TabName;
        instance_guid                           m_InstanceGuid;
        const ImGuiWindowClass*                 m_pParentImGuiClass { nullptr };
        bool                                    m_OpenPanel         { true };
        bool                                    m_bPanelVisible     { true };
        bool                                    m_bSetActive        { false };
        bool                                    m_bFirstTimeRender  { true };
    };

    //--------------------------------------------------------------------------------------

    class child : public base
    {
        xcore_rtti(child, base);
    
    public:

        struct construct : base::construct
        {
            parent&                         m_Parent;
        };

    public:

                                        child                       ( construct& Construct );
                        void            Render                      ( void );
    
    protected:

        virtual         void            onLogic                     ( void ) {}
        virtual         void            onRender                    ( void ) = 0;
        virtual         void            onCloseProject              ( void ) {}
    
    protected:

        parent& m_Parent;

        friend class  xeditor::frame;
    };

    //--------------------------------------------------------------------------------------

    class parent : public base
    {
        xcore_rtti(parent, base);
    
    public:

        struct construct : base::construct
        {
            xeditor::frame& m_EditorFrame;
        };

    public:
                                                parent              ( construct& Construct );
                        void                    Render              ( void );
        inline         xeditor::document::main& getMainDoc          ( void );
        inline          const ImGuiWindowClass& getImGuiClass       ( void ) const noexcept;
                        xcore::err              CreateTab           ( const char* pString, bool bActive = true ) noexcept;
    
    protected:

        virtual         void            onLogic                     ( void ) {}
        virtual         void            onRender                    ( void ) = 0;
        virtual         void            onCloseProject              ( void ) {}
        virtual         void            onCreateChildrenPanels      ( void ) {}

    protected:

        xeditor::frame&                         m_AppFrame;
        xcore::vector<std::unique_ptr<child>>   m_lChildPanels  {};
        ImGuiWindowClass                        m_ImGuiClass    {};

        friend class  xeditor::frame;
    };

}
