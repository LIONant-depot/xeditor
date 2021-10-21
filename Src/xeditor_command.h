namespace xeditor::command
{
    struct undo_step
    {
                                undo_step       ( void );
                               ~undo_step       ( void );

        void                    Clear           ( void );

        std::int32_t            m_iStep             { -1 };
        xcore::cstring          m_Command           {};
        xcore::file::stream     m_UndoStateFile     {};
        xcore::file::stream     m_RedoStateFile     {};
    };

    //------------------------------------------------------------------------------

    class undo_system
    {
    public:

        ~undo_system(void);

        void                    Init            ( std::int32_t nMaxStepCount, const char* pFilePath );
        void                    Kill            ( void );
        void                    Reset           ( void );
        bool                    CanUndo         ( std::int32_t nSteps = 1 ) const;
        bool                    CanRedo         ( std::int32_t nSteps = 1 ) const;

        undo_step&              UndoStep        ( void );
        undo_step&              RedoStep        ( void );
        undo_step&              getNewUndoStep  ( void );

    protected:

        constexpr               undo_system     ( void ) = default;

        std::int32_t            getPrev         ( void ) const;
        std::int32_t            getNext         ( void ) const;
        xcore::cstring          getUndoFileName ( std::int32_t iStep ) const;
        xcore::cstring          getRedoFileName ( std::int32_t iStep ) const;
        void                    CreateFiles     ( undo_step& Step );
        void                    DeleteFiles     ( undo_step& Step );
        void                    ResetStep       ( undo_step& Step );

    protected:

        xcore::unique_span<undo_step>   m_Stack     {};
        std::int32_t                    m_iCurrent  { 1 };
        xcore::cstring                  m_FilePath  {};

        friend class system;
    };

    //------------------------------------------------------------------------------

    class base
    {
    public:

                                base ( system& System ) : m_System{ System } {}
        virtual                ~base ( void );

        virtual bool            ValidateCommand     ( const char* pCmdline )        = 0;
        virtual bool            ExecuteCommand      ( undo_step& Step )             = 0;
        virtual bool            LoadUndoState       ( xcore::file::stream& File )   = 0;
        virtual bool            LoadRedoState       ( xcore::file::stream& File )   = 0;
        virtual const char*     getHelp             ( void ) const                  = 0;
        virtual const char*     getName             ( void ) const                  = 0;

    protected:

        system& m_System;
    };

    //------------------------------------------------------------------------------

    class system
    {
    public:

        constexpr               system              ( void ) = default;
                               ~system              ( void );

        virtual void            Init                ( std::int32_t UndoSteps, const char* pUndoPath );
        virtual void            Kill                ( void );
        virtual void            Reset               ( void );
        virtual bool            RegisterCommand     ( base& Command );
        virtual bool            ExecuteCommand      ( const char* pCmdline );
        virtual bool            Undo                ( void );
        virtual bool            Redo                ( void );
        virtual bool            CanUndo             ( void ) const;
        virtual bool            CanRedo             ( void ) const;

    public:

                void            InitUndoSystem      ( std::int32_t nUndoSize, const char* pUndoPath );

    protected:

        virtual bool            isCommandRegistered ( const base& Command ) const;
        virtual base*           findCommand         ( const char* pName ) const;
        virtual bool            isHelpCommand       ( const char* pName ) const;
        virtual bool            ExecuteHelpCommand  ( const char* pCmdline ) const;

    protected:

        xcore::vector<base*>        m_CommandArray      {};
        undo_system                 m_UndoSystem        {};

        std::int32_t                m_nUndoSystemSize   { 50 };
        xcore::cstring              m_pUndoSystemPath   {};
    };
}
