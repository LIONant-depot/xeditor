namespace xeditor::document
{
    using                   group_guid  = xcore::guid::group< struct xeditor_document_tag >;
    constexpr static auto   type_guid_v = group_guid{ "base::document" };
    using                   guid        = xcore::guid::subgroup<group_guid>;

    //--------------------------------------------------------------------------------------------

    struct type
    {
                                                    type        ( xcore::string::constant<char> Str, guid ClassGuid, int Weight = 0 )           noexcept;
        static                  type*&              getHead     ( void )                                                                        noexcept;
        virtual                 void                Construct   ( base& Base, xcore::string::constant<char> Str, main& MainDoc )        const   noexcept = 0;
        virtual                 base*               Malloc      ( void )                                                                const   noexcept = 0;

        type*                                   m_pNext         { nullptr };
        const xcore::string::constant<char>     m_TypeName;
        guid                                    m_Guid;
        const int                               m_Weight;
    };

    //--------------------------------------------------------------------------------------------

    class base 
        : public    xeditor::command::system
        , protected xcore::system::registration
        , protected xcore::property::base
    {
        property_vtable();
        xcore_rtti_start( base )

    public:

        using guid = xeditor::document::guid;

    public:

                                                        base                ( xcore::string::constant<char> Str, main& MainDoc ) noexcept;
        virtual                                        ~base                ( void ) = default;
        inline          xeditor::document::main&        getMainDoc          ( void )                                        noexcept;// { return m_MainDoc.SafeCast<T>(); }
        inline          const xeditor::document::main&  getMainDoc          ( void )                                const   noexcept;// { return m_MainDoc.SafeCast<T>(); }
        virtual         const type&                     getType             ( void )                                const   noexcept = 0;

    public:

        base*                           m_pNext      { nullptr };
        xcore::cstring                  m_DocName    {};
        xcore::log::channel             m_LogChannel { "xeditor" };

    protected:

        virtual         xcore::err             onSave                  ( void ) = 0;
        virtual         xcore::err             onLoad                  ( void ) = 0;
        virtual         xcore::err             onNew                   ( void ) = 0;
        virtual         xcore::err             onClose                 ( void ) = 0;
        virtual         xcore::err             onSaveProjectSettings   ( void );
        virtual         xcore::err             onLoadProjectSettings   ( void );
        virtual         xcore::err             onSaveEditorSettings    ( void );
        virtual         xcore::err             onLoadEditorSettings    ( void );

    protected:

        main&                                                   m_MainDoc;

        friend class main;
    };
}