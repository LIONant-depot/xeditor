namespace xeditor::ui
{
    void    ToolTip                 ( const char* pfmt );
    void    MenuToolTip             ( const char* pfmt );
    void    MenuInfo                ( const char* pfmt );
    void    MenuSameLineAfterIcon   ( void );
    bool    MenuSwitchButton        ( bool bState, const char* pButtonName, const char* pHelp = nullptr );
    bool    CircleButton            ( void );
    bool    MenuItem                ( const char* pName, const char* pHelp );
    void    Separator               ( void );

    template< typename T >  
    void    PulldownMenu            ( const char* pString, T&& Function );

    template< typename T >  
    void    ChildWindow             ( const char* pString, T&& Function );

    template< typename T_PROCESS >
    bool    TreeFolder              ( const void* pID, bool& B, const char* pString, T_PROCESS&& Function, ImGuiTreeNodeFlags Flags = 0 );

    template< typename T_ID, typename T_PROCESS, typename T_CONTEXT_MENU >
    bool    TreeFolder              ( const T_ID& ID, bool& B, const char* pString, T_PROCESS&& Function, T_CONTEXT_MENU&& Context, ImGuiTreeNodeFlags Flags = 0 );

    inline 
    void    TreeItem                ( const void* pID, const char* pString, ImGuiTreeNodeFlags Flags = 0 );

    template< typename V, typename T >
    void    ContextMenu             ( const char* pStr, const V& Var, T&& Function );

    template< typename T_VARIANT_TYPE, typename V, typename T >
    bool    ContextMenuPost         ( const char* pStr, V& Params, T&& Function );

    template< typename T >
    void    PulldownCircleMenu      ( T&& Function );

    //-------------------------------------------------------------------------------------------

    struct name_dlg
    {
        enum class state
        {
            RUNNING
            , OK
            , CANCEL
            , NOT_OPEN
        };

        state m_State { state::NOT_OPEN };
        void operator() ( const char* pName, std::span<char> Buffer );
    };
}