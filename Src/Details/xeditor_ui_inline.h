namespace xeditor::ui
{
    //-------------------------------------------------------------------------------------------

    template< typename T >
    inline void PulldownMenu( const char* pString, T&& Function )
    {
        if( ImGui::BeginMenu( pString ) )
        {
            Function();
            ImGui::EndMenu();
        }
    }

    //-------------------------------------------------------------------------------------------

    template< typename T >
    inline void ChildWindow( const char* pString, T&& Function )
    {
        ImGui::BeginChild( pString, ImVec2(0,0), true );
        Function();
        ImGui::EndChild();
    }

    //-------------------------------------------------------------------------------------------

    template< typename T_PROCESS >
    inline bool TreeFolder( const void* pID, bool& B, const char* pString, T_PROCESS&& Function, ImGuiTreeNodeFlags Flags )
    {
        B = ImGui::TreeNodeEx( pID
                             , Flags | ( B ? ImGuiTreeNodeFlags_DefaultOpen : 0 )
                             , pString );
        if ( B )
        {
            Function();
            ImGui::TreePop();
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------

    template< typename T_ID, typename T_PROCESS, typename T_CONTEXT_MENU >
    inline bool TreeFolder( const T_ID& ID, bool& B, const char* pString, T_PROCESS&& Function, T_CONTEXT_MENU&& Context, ImGuiTreeNodeFlags Flags )
    {
        B = ImGui::TreeNodeEx( (const void*) &ID
                               , Flags | ( B ? ImGuiTreeNodeFlags_DefaultOpen : 0 )
                               , pString );
        Context( ID );

        if( B )
        {
            Function();
            ImGui::TreePop();
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------

    inline void TreeItem( const void* pID, const char* pString, ImGuiTreeNodeFlags Flags )
    {
        Flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf;
        ImGui::TreeNodeEx( pID, Flags, pString );
    }

    //-------------------------------------------------------------------------------------------
    
    template< typename V, typename T >
    void ContextMenu( const char* pStr, const V& Var, T&& Function )
    {
        if constexpr ( std::is_pointer_v<V> )
        {
            ImGui::PushID( Var );
        }
        else
        {
            ImGui::PushID( &Var );
        }

        if ( ImGui::BeginPopupContextItem( pStr ) )
        {
            Function( Var );
            ImGui::EndPopup();
        }
        ImGui::PopID();
    };

    //-------------------------------------------------------------------------------------------

    template< typename T_VARIANT_TYPE, typename V, typename T >
    bool ContextMenuPost( const char* pStr, V& Params, T&& Function )
    {
        if( Params.template isType<T_VARIANT_TYPE>() == false ) 
            return false;

        auto& P = Params.get<T_VARIANT_TYPE>();
        if( P.m_bOpen == false )
        {
            ImGui::OpenPopup( pStr );
            P.m_bOpen = true;
        }

        if( ImGui::BeginPopupContextItem( pStr ) )
        {
            Function( P );
            ImGui::EndPopup();
        }

        return true;
    };

    //-------------------------------------------------------------------------------------------

    template< typename T >
    void PulldownCircleMenu( T&& Function )
    {
        if( CircleButton() )
        {
            ImGui::OpenPopup("The Circle Popup");
        }

        ImGui::SameLine( 35 );

        if( ImGui::BeginPopupContextItem( "The Circle Popup" ) )
        {
            Function();
            ImGui::EndPopup();
        }
    }

    //-------------------------------------------------------------------------------------------
    //-------------------------------------------------------------------------------------------

    namespace details
    {
        void        CircleButtonStylingPush(void);
        void        CircleButtonStylingPop(void);
    }
}