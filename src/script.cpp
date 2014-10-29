#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/busyinfo.h>
#include <wx/progdlg.h>

#include <wex/lkscript.h>
#include <wex/metro.h>
#include <wex/utils.h>

#include "casewin.h"
#include "library.h"
#include "main.h"
#include "invoke.h"
#include "simulation.h"
#include "script.h"
#include "urdb.h"

static void fcall_open_project( lk::invoke_t &cxt )
{
	LK_DOC( "open_project", "Open a SAM project file.", "(string:file):boolean" );

	if ( !SamApp::Window()->CloseProject() ) cxt.result().assign( 0.0 );
	else cxt.result().assign( SamApp::Window()->LoadProject( cxt.arg(0).as_string() ) ? 1.0 : 0.0 );
}

static void fcall_create_case( lk::invoke_t &cxt )
{
	LK_DOC( "create_case", "Create a new case.", "(string:technology, string:financing, [string:name]):boolean" );
	wxString name;
	if ( cxt.arg_count() == 3 )
		name = cxt.arg(2).as_string();

	cxt.result().assign( SamApp::Window()->CreateNewCase( name, 
			cxt.arg(0).as_string(), 
			cxt.arg(1).as_string() ) ? 1.0 : 0.0 );
}

static void fcall_close_project( lk::invoke_t &cxt )
{
	LK_DOC( "close_project", "Closes the current SAM project, if one is open.", "(none):boolean" );
	cxt.result().assign( SamApp::Window()->CloseProject() ? 1.0 : 0.0 );
}

static void fcall_save_project( lk::invoke_t &cxt )
{
	LK_DOC( "save_project", "Saves the current SAM project to disk.", "(string:file):boolean" );
	cxt.result().assign( SamApp::Window()->SaveProject( cxt.arg(0).as_string() ) ? 1.0 : 0.0 );
}

static void fcall_project_file( lk::invoke_t &cxt )
{
	LK_DOC( "project_file", "Returns the file name of the current project.", "(none):string" );
	cxt.result().assign( SamApp::Window()->GetProjectFileName() );
}

static void fcall_list_cases( lk::invoke_t &cxt )
{
	LK_DOC( "list_cases", "Return a list of the case names in the current project.", "(none):array" );
	cxt.result().empty_vector();
	wxArrayString names = SamApp::Window()->Project().GetCaseNames();
	for( size_t i=0;i<names.size();i++ )
		cxt.result().vec_append( names[i] );
}

static Case *CurrentCase() { return SamApp::Window()->GetCurrentCase(); }

static void fcall_active_case( lk::invoke_t &cxt )
{
	LK_DOC( "active_case", "Sets the currently active case, or returns its name.", "([string:case name]):variant" );
	if (cxt.arg_count() == 0 )
		cxt.result().assign( SamApp::Window()->Project().GetCaseName( CurrentCase() ) );
	else
		cxt.result().assign( SamApp::Window()->SwitchToCaseWindow( cxt.arg(0).as_string() ) ? 1.0 : 0.0 );	
}

static void fcall_varinfo( lk::invoke_t &cxt )
{
	LK_DOC("varinfo", "Gets meta data about an input or output variable.", "(string:var name):table");
	wxString name = cxt.arg(0).as_string();
	cxt.result().empty_hash();
	if ( Case *c = CurrentCase() )
	{
		if (VarInfo *vi = c->Variables().Lookup( name ))
		{
			cxt.result().hash_item("label").assign( vi->Label );
			cxt.result().hash_item("units").assign( vi->Units );
			cxt.result().hash_item("group").assign( vi->Group );
		}
		else
		{
			wxArrayString names, labels, units, groups;
			Simulation::ListAllOutputs( c->GetConfiguration(),
				&names, &labels, &units, &groups );
			int idx = names.Index( name );
			if ( idx >=0 )
			{
				cxt.result().hash_item("label").assign( labels[idx] );
				cxt.result().hash_item("units").assign( units[idx] );
				cxt.result().hash_item("group").assign( groups[idx] );
			}
		}
	}
}

static void fcall_selectinputs( lk::invoke_t &cxt )
{
	LK_DOC("select_inputs", "Shows the input variable selection dialog.", "(<array:checked>, [string:title]):boolean")
		
	cxt.result().assign( 0.0 );

	Case *cc = CurrentCase();
	if ( !cc ) return;

	wxArrayString names;
	wxArrayString labels;

	for( VarInfoLookup::iterator it = cc->Variables().begin();
		it != cc->Variables().end();
		++it )
	{
		VarInfo &vi = *(it->second);

		if ( !vi.Label.IsEmpty()
			&& !(vi.Flags & VF_INDICATOR) 
			&& !(vi.Flags & VF_CALCULATED) )
		{
			if ( vi.Type == VV_NUMBER && vi.IndexLabels.size() > 0 )
				continue;

			wxString label;
			if ( !vi.Group.IsEmpty() )
				label = vi.Group + "/" + vi.Label;
			else
				label = vi.Label;
							
			names.Add( it->first );
			labels.Add( label );
		}
	}

	wxSortByLabels(names, labels);

	wxArrayString list;
	lk::vardata_t &inlist = cxt.arg(0).deref();
	for( size_t i=0;i<inlist.length();i++ )
		list.Add( inlist.index(i)->as_string() );

	wxString title("Select input variables");
	if (cxt.arg_count() > 1 ) title = cxt.arg(1).as_string();

	if ( SelectVariableDialog::Run(title, names, labels, list) )
	{
		inlist.empty_vector();
		for( size_t i=0;i<list.size();i++ )
			inlist.vec_append( list[i] );

		cxt.result().assign( 1.0 );
	}
}


static void fcall_set( lk::invoke_t &cxt )
{
	LK_DOC( "set", "Set an input variable's value.", "(string:name, variant:value):boolean" );
	cxt.result().assign( 0.0 );
	wxString name = cxt.arg(0).as_string();
	if ( Case *c = CurrentCase() )
	{
		if ( VarValue *vv = c->Values().Get( name ) )
		{
			bool ok = vv->Read( cxt.arg(1) );
			c->VariableChanged( name );		
			cxt.result().assign( ok ? 1.0 : 0.0 );
		}
	}
}

static void fcall_get( lk::invoke_t &cxt )
{
	LK_DOC("get", "Get an output or input variable's value.", "(string:name):variant" );
	if ( Case *c = CurrentCase() )
	{
		wxString name = cxt.arg(0).as_string();
		if ( VarValue *vv = c->BaseCase().GetOutput( name ) )
			vv->Write( cxt.result() );
		else if ( VarValue *vv = c->Values().Get( name ) )
			vv->Write( cxt.result() );
	}
}


static bool sg_scriptSimCancel;
class ScriptSimulationHandler : public ISimulationHandler
{
public:
	ScriptSimulationHandler() {
		sg_scriptSimCancel = false;
	}
	
	virtual ~ScriptSimulationHandler() {
	}

	virtual void Update( float percent, const wxString &text )
	{
		wxGetApp().Yield( true );
	}

	virtual bool IsCancelled()
	{
		return sg_scriptSimCancel;
	}

	static void Cancel() { sg_scriptSimCancel = true; }
};


void ScriptWindow::CancelRunningSimulations()
{
	ScriptSimulationHandler::Cancel();
}

static void fcall_simulate( lk::invoke_t &cxt )
{
	LK_DOC("simulate", "Run the base case simulation for the currently active case.  Errors and warnings are optionally returned in the first parameter.  The results in the user interface are not updated by default, but can be via the second parameter.", "( [string:messages], [boolean: update UI] ):boolean" );
	cxt.result().assign( 0.0 );
	if ( Case *c = CurrentCase() )
	{
		Simulation &bcsim = c->BaseCase();		
		bcsim.Clear();

		ExcelExchange &ex = c->ExcelExch();
		if ( ex.Enabled )
			ExcelExchange::RunExcelExchange( ex, c->Values(), &bcsim );

		if ( !bcsim.Prepare() )
		{
			cxt.result().assign( 0.0 );
			
			if( cxt.arg_count() > 0 )
				cxt.arg(0).assign(  wxJoin( c->BaseCase().GetAllMessages(), '\n' ) );

			return;
		}
		
		ScriptSimulationHandler ssh;		
		cxt.result().assign( bcsim.InvokeWithHandler( &ssh ) ? 1.0 : 0.0 );

		if( cxt.arg_count() > 0 )
			cxt.arg(0).assign(  wxJoin( c->BaseCase().GetAllMessages(), '\n' ) );

		if ( cxt.arg_count() > 1 && cxt.arg(1).as_boolean() )
			if ( CaseWindow *cw = SamApp::Window()->GetCaseWindow( c ) )
					cw->UpdateResults();
	}
}

static void fcall_configuration( lk::invoke_t &cxt )
{
	LK_DOC( "configuration", "Change the current active case's technology/market configuration, or return the current configuration.", "(string:technology, string:financing):boolean or (none):array");

	if ( cxt.arg_count() == 2 )
	{
		wxString tech = cxt.arg(0).as_string();
		wxString fin = cxt.arg(1).as_string();

		cxt.result().assign( 0.0 );
		wxArrayString techlist = SamApp::Config().GetTechnologies();
		if ( techlist.Index( tech ) == wxNOT_FOUND ) return;
		wxArrayString finlist = SamApp::Config().GetFinancingForTech( tech );
		if ( finlist.Index( fin ) == wxNOT_FOUND ) return;

		if ( Case *c = CurrentCase() )
			cxt.result().assign( c->SetConfiguration( tech, fin, true, 0 ) ? 1.0 : 0.0 ); // invoke silently - do not show error messages
	}
	else if ( Case *c = CurrentCase() )
	{
		cxt.result().empty_vector();
		cxt.result().vec_append( c->GetTechnology() );
		cxt.result().vec_append( c->GetFinancing() );
	}
}

static void fcall_load_defaults( lk::invoke_t &cxt )
{
	LK_DOC( "load_defaults", "Load SAM default values for the current active case. An optional error message is stored in the first argument, if given.", "([<string:error>]):boolean" );
	if ( Case *c = CurrentCase() )
	{
		wxString err;
		if ( c->LoadDefaults( &err ) )
			cxt.result().assign( 1.0 );
		else
		{
			if ( cxt.arg_count() == 1 )
				cxt.arg(0).assign( err );

			cxt.result().assign( 0.0 );
		}
	}
	else
		cxt.result().assign( 0.0 );
}

static void fcall_overwrite_defaults( lk::invoke_t &cxt )
{
	LK_DOC( "overwrite_defaults", "Overwrite SAM default values file for the current configuration with current values.", "(none):boolean");
	if ( Case *c = CurrentCase() )
		cxt.result().assign( c->SaveDefaults( true ) );
}

static void fcall_list_technologies( lk::invoke_t &cxt )
{
	LK_DOC( "list_technologies", "List available technology options in SAM.", "(none):array" );
	wxArrayString list = SamApp::Config().GetTechnologies();
	cxt.result().empty_vector();
	for( size_t i=0;i<list.size();i++ )
		cxt.result().vec_append( list[i] );
}

static void fcall_list_financing( lk::invoke_t &cxt )
{
	LK_DOC( "list_financing", "List available financial model options for a particular technology.", "(string:technology):array" );
	wxArrayString list = SamApp::Config().GetFinancingForTech( cxt.arg(0).as_string() );
	cxt.result().empty_vector();
	for( size_t i=0;i<list.size();i++ )
		cxt.result().vec_append( list[i] );
}

static void fcall_library( lk::invoke_t &cxt )
{
	LK_DOC( "library", "Obtain a list of library types (no arguments), or all entries for a particular type (1 argument).", "( [string:lib type] ):array" );
	
	wxArrayString list;
	if ( cxt.arg_count() == 0 )
		list = Library::ListAll();
	else if ( Library *lib = Library::Find( cxt.arg(0).as_string() ) )
		list = lib->ListEntries();

	cxt.result().empty_vector();
	for( size_t i=0;i<list.size();i++ )
		cxt.result().vec_append( list[i] );
}

/* 
	tab->Add( ChangeConfig, "ChangeConfig", 2, "Changes the current case's configuration. Application must be '*'.", "( STRING:Technology, STRING:Financing ):BOOLEAN");
	tab->Add( ReloadDefaults, "ReloadDefaults", 0, "Reloads all default values for the active case.", "( NONE ):NONE");
	tab->Add( ListTechnologies, "ListTechnologies", 0, "Returns an array of all the technologies in SAM.", "( NONE ):ARRAY");
	tab->Add( ListFinancing, "ListFinancing", 1, "Lists all financing options in SAM for a given technology.", "( STRING:Technology ):ARRAY");
	pptab->Add( OverwriteDefaults, "OverwriteDefaults", 1, "Overwrites the existing defaults file with the current case inputs for the specified case.", "( STRING:Case name ):BOOLEAN");
	tab->Add( ListCases, "ListCases", 0, "Lists all the cases in the project.", "( NONE ):ARRAY");
	tab->Add( TechnologyType, "TechnologyType", 0, "Returns the active case technology type.", "( NONE ):STRING");
	tab->Add( FinancingType, "FinancingType", 0, "Returns the active case financing type.", "( NONE ):STRING");

	// remaining

	pptab->Add( CurrentCaseName, "CurrentCaseName", 0, "Returns the currently selected case's name.", "( NONE ):STRING");
	pptab->Add( RerunCase, "RerunCase", 1, "Resimulates all setups for the specified case.", "( STRING:Case name ):BOOLEAN");
	tab->Add( ResetOutputSource, "ResetOutputSource", -1, "Resets the output data source to default BASE case, or changes it to a different simulation and run number.", "( NONE or STRING:Simulation name, INTEGER:Run number ):NONE");
	tab->Add( ClearSimResults, "ClearSimResults", 1, "Clears all results for the specific simulation name.", "( STRING:Simulation name ):NONE");
	tab->Add( SwitchToCase, "SwitchToCase", 0, "Switches to the active case tab in the interface.", "( NONE ):NONE");
	tab->Add( SamDir, "SamDir", 0, "Returns the SAM installation folder on the local computer.", "( NONE ):STRING");
	tab->Add( MPSimulate, "MPSimulate", 2, "Runs many simulations using multiple processors.", "( STRING:Simulation name, ARRAY[ARRAY]:Variable name/value table NRUNS+1 x NVARS with top row having var names ):BOOLEAN" );
	tab->Add( WriteResults, "WriteResults", 2, "Write a comma-separated-value file, with each column specified by a string of comma-separated output names.", "( STRING:File name, STRING:Comma-separated output variable names):BOOLEAN");
	tab->Add( ClearResults, "ClearResults", 0, "Clear the active case's results from memory.", "( NONE ):NONE");
	tab->Add( ClearCache, "ClearCache", 0, "Clear the memory cache of previously run simulations.", "( NONE ):NONE");
	tab->Add( DeleteTempFiles, "DeleteTempFiles", 0, "Delete any lingering simulation temporary files.", "( NONE ):NONE");
	tab->Add( SetTimestep, "SetTimestep", 1, "Sets the TRNSYS timestep for the active case.", "( STRING:Timestep with units ):NONE");
	tab->Add( ActiveVariables, "ActiveVariables", -1, "List all active variables for the current case or technology/market name.", "( [STRING:Technology, STRING:Financing] ):ARRAY");
	tab->Add( FlDensity, "FluidDensity", 2, "Returns density at temperature Tc for a given fluid number (pressure assumed 1Pa).", "( INTEGER:Fluid number, DOUBLE:Temp 'C ):DOUBLE");
	tab->Add( FlSpecificHeat, "FluidSpecificHeat", 2, "Returns specific heat at temperature Tc for a given fluid number (pressure assumed 1Pa).", "( INTEGER:Fluid number, DOUBLE:Temp 'C ):DOUBLE");
	tab->Add( FlName, "FluidName", 1, "Returns fluid name for a given fluid number.", "( INTEGER:Fluid number ):STRING");
	tab->Add( PtOptimize, "PtOptimize", 0, "Optimizes the power tower heliostat field, tower height, receiver height, and receiver diameter.  Returns whether the optimization succeeded.", "( NONE ):BOOLEAN");
	tab->Add( PtGetOutput, "PtGetOutput", 0, "Returns any output or error messages from the PTGen solar field optimization routine.", "(NONE):STRING");
	tab->Add( Coeffgen6par, "Coeffgen6par", 9, "Calculates the 6 parameters for the CEC 6 parameter model", "(STRING:cell type, DOUBLE:Vmp, DOUBLE:Imp, DOUBLE:Voc, DOUBLE:Isc, DOUBLE:beta, DOUBLE:alpha, DOUBLE:gamma, INTEGER:nser):ARRAY[a,Io,Il,Rs,Rsh,Adj] or false on failure");
	tab->Add( SetTrnsysOutputFolder, "SetTrnsysOutputFolder", 1, "Sets the output folder for hourly TRNSYS output data and files.", "(STRING:path):NONE");
	tab->Add( Library, "Library", -1, "Obtain a list of library types (no arguments), or all entries for a particular type (1 argument).", "([STRING:type]):ARRAY");
	tab->Add( Pearson, "Pearson", -1, "Calculates the linear (pearson) correlation coefficient between two arrays of the same length.", "(ARRAY:x, ARRAY:y):DOUBLE");

	tab->Add( LHS_create, "LHSCreate", 0, "Creates a new Latin Hypercube Sampling object.", "( NONE ):INTEGER");
	tab->Add( LHS_free, "LHSFree", 1, "Frees an LHS object.", "( INTEGER:lhsref ):NONE");
	tab->Add( LHS_reset, "LHSReset", 1, "Erases all distributions and correlations in an LHS object.", "( INTEGER:lhsref ):NONE");
	tab->Add( LHS_seed, "LHSSeed", 2, "Sets the seed value for the LHS object.", "( INTEGER:seed ):NONE");
	tab->Add( LHS_points, "LHSPoints", 2, "Sets the number of samples desired.", "( INTEGER:lhsref, INTEGER:number of points ):NONE");
	tab->Add( LHS_dist, "LHSDist", -1, "Sets up a distribution for a variable.", "( INTEGER:lhsref, STRING:distribution name, STRING: variable name, [DOUBLE:param1, DOUBLE:param2, DOUBLE:param3, DOUBLE:param4] ): NONE");
	tab->Add( LHS_corr, "LHSCorr", 4, "Sets up correlation between two variables.", "( INTEGER:lhsref, STRING:variable 1, STRING:variable 2, DOUBLE:corr val ):NONE");
	tab->Add( LHS_run, "LHSRun", 1, "Runs the LHS sampling program.", "( INTEGER:lhsref ):BOOLEAN");
	tab->Add( LHS_error, "LHSError", 1, "Returns an error message if any.", "( INTEGER:lhsref ):STRING");
	tab->Add( LHS_vector, "LHSVector", 2, "Returns the sampled values for a variable.", "( INTEGER:lhsref, STRING:variable ):ARRAY");

	tab->Add( STEPWISE_create, "STEPCreate", 0, "Create a new STEPWISE regression analysis object.", "( NONE ):INTEGER");
	tab->Add( STEPWISE_free, "STEPFree", 1, "Frees a STEPWISE object.", "( INTEGER:stpref ):NONE" );
	tab->Add( STEPWISE_input, "STEPInput", 3, "Sets a STEPWISE input vector.", "( INTEGER:stpref, STRING:name, ARRAY:values ):NONE");
	tab->Add( STEPWISE_output, "STEPOutput", 2, "Sets a STEPWISE output vector.", "( INTEGER:stpref, ARRAY:values ):NONE");
	tab->Add( STEPWISE_run, "STEPRun", 1, "Runs the STEPWISE analysis.", "( INTEGER:stpref ):NONE" );
	tab->Add( STEPWISE_error, "STEPError", 1, "Returns any error code from STEPWISE.", "( INTEGER:stpref ):STRING" );
	tab->Add( STEPWISE_result, "STEPResult", 2, "Returns R2 and SRC for a given input name.", "( INTEGER:stpref, STRING:name ):ARRAY");

	*/

// external fcalls that are compatible with running in a script environment
extern void fcall_urdb_read( lk::invoke_t & );
extern void fcall_urdb_write( lk::invoke_t & );
extern void fcall_urdb_get( lk::invoke_t & );
extern void fcall_urdb_list_utilities( lk::invoke_t & );
extern void fcall_urdb_list_rates( lk::invoke_t & );

lk::fcall_t *sam_functions() {
	
	static const lk::fcall_t vec[] = {
		fcall_open_project,
		fcall_close_project,
		fcall_save_project,
		fcall_create_case,
		fcall_project_file,
		fcall_list_cases,
		fcall_active_case,
		fcall_varinfo,
		fcall_selectinputs,
		fcall_set,
		fcall_get,
		fcall_simulate,
		fcall_configuration,
		fcall_library,
		fcall_load_defaults,
		fcall_overwrite_defaults,
		fcall_list_technologies,
		fcall_list_financing,
		fcall_urdb_read,
		fcall_urdb_write,
		fcall_urdb_get,
		fcall_urdb_list_utilities,
		fcall_urdb_list_rates,
		0 };
	return (lk::fcall_t*)vec;

};

class SamScriptCtrl : public wxLKScriptCtrl
{
	ScriptWindow *m_scriptwin;
public:
	SamScriptCtrl( wxWindow *parent, int id, ScriptWindow *scriptwin )
		: wxLKScriptCtrl( parent, id, wxDefaultPosition, wxDefaultSize,
			(wxLK_STDLIB_BASIC|wxLK_STDLIB_STRING|wxLK_STDLIB_MATH|wxLK_STDLIB_WXUI|wxLK_STDLIB_PLOT|wxLK_STDLIB_MISC|wxLK_STDLIB_SOUT) ),
		 m_scriptwin( scriptwin )
	{
		ShowFindInFilesButton( true );

		// register SAM-specific invoke functions here
		RegisterLibrary( invoke_general_funcs(), "General Functions" );
		RegisterLibrary( invoke_ssc_funcs(), "Direct Access To SSC" );
		RegisterLibrary( sam_functions(), "SAM Functions");
	}

	virtual bool OnFindInFiles( const wxString &text, bool match_case, bool whole_word )
	{;
		std::vector<ScriptWindow*> windows = m_scriptwin->GetWindows();
		
		wxProgressDialog dialog( "Find in files", "Searching for " + text, (int)windows.size(), m_scriptwin,
			wxPD_SMOOTH|wxPD_CAN_ABORT );
		dialog.SetClientSize( wxSize(350,100) );
		dialog.CenterOnParent();
		dialog.Show();

		m_scriptwin->ClearOutput();
		int noccur = 0;
		for( size_t i=0;i<windows.size();i++ )
		{
			ScriptWindow *sw = windows[i];

			int iter = 0;
			int pos, line_num;
			wxString line_text;
			while( sw->Find( text, match_case, whole_word,
				iter == 0, &pos, &line_num, &line_text ) )
			{
				m_scriptwin->AddOutput( sw->GetTitle() + "  (" + wxString::Format("%d):  ", line_num) + line_text );
				noccur++;
				iter++;
			}

			if ( !dialog.Update( i ) )
				break;
		}

		m_scriptwin->AddOutput( wxString::Format("\n%d files searched, %d occurences found.", (int)windows.size(), noccur) );

		return true;
	}
	
	virtual bool OnEval( int line )
	{
		wxGetApp().Yield( true );
		return true;
	}

	virtual void OnOutput( const wxString &out )
	{
		m_scriptwin->AddOutput( out );
	}
};

enum { ID_SCRIPT = wxID_HIGHEST+494 ,
	ID_VARIABLES, ID_FUNCTIONS, ID_OUTPUT_TEXT };

BEGIN_EVENT_TABLE( ScriptWindow, wxFrame )
	EVT_BUTTON( wxID_NEW, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_OPEN, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_SAVE, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_SAVEAS, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_FIND, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_EXECUTE, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_STOP, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_CLOSE, ScriptWindow::OnCommand )
	EVT_BUTTON( wxID_HELP, ScriptWindow::OnCommand )
	EVT_BUTTON( ID_VARIABLES, ScriptWindow::OnCommand )
	EVT_BUTTON( ID_FUNCTIONS, ScriptWindow::OnCommand )
	
	EVT_MENU( wxID_NEW, ScriptWindow::OnCommand )
	EVT_MENU( wxID_OPEN, ScriptWindow::OnCommand )
	EVT_MENU( wxID_SAVE, ScriptWindow::OnCommand )
	EVT_MENU( wxID_FIND, ScriptWindow::OnCommand )
	EVT_MENU( wxID_EXECUTE, ScriptWindow::OnCommand )
	EVT_MENU( wxID_CLOSE, ScriptWindow::OnCommand )
	EVT_MENU( wxID_HELP, ScriptWindow::OnCommand )

	EVT_STC_MODIFIED( ID_SCRIPT, ScriptWindow::OnModified )
	EVT_CLOSE( ScriptWindow::OnClose )
END_EVENT_TABLE()


ScriptWindow::ScriptWindow( wxWindow *parent, int id, const wxPoint &pos, const wxSize &size )
	: wxFrame( parent, id, wxT("untitled"), pos, size ) 
{
	m_lastFindPos = 0;

#ifdef __WXMSW__
	SetIcon( wxICON( appicon ) );
#endif	
	SetBackgroundColour( wxMetroTheme::Colour(wxMT_FOREGROUND) );
	
#ifdef __WXOSX__
	wxMenu *file = new wxMenu;
	file->Append( wxID_NEW );
	file->AppendSeparator();
	file->Append( wxID_OPEN );
	file->Append( wxID_SAVE );
	file->Append( wxID_SAVEAS );
	file->AppendSeparator();
	file->Append( wxID_EXECUTE );
	file->AppendSeparator();
	file->Append( wxID_CLOSE );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( file, wxT("&File") );
	SetMenuBar( menuBar );
#endif	

	wxBoxSizer *toolbar = new wxBoxSizer( wxHORIZONTAL );
	toolbar->Add( new wxMetroButton( this, wxID_NEW, "New" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_OPEN, "Open" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_SAVE, "Save" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_SAVEAS, "Save as" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_FIND, "Find" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( m_runBtn=new wxMetroButton( this, wxID_EXECUTE, "Run", wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxMB_RIGHTARROW ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( m_stopBtn=new wxMetroButton( this, wxID_STOP, "Stop" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->AddStretchSpacer();
	toolbar->Add( new wxMetroButton( this, ID_VARIABLES, "Variables" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, ID_FUNCTIONS, "Functions" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_HELP, "Help" ), 0, wxALL|wxEXPAND, 0 );
	toolbar->Add( new wxMetroButton( this, wxID_CLOSE, "Close" ), 0, wxALL|wxEXPAND, 0 );

	m_stopBtn->Hide();

	wxSplitterWindow *split = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE|wxSP_3DSASH|wxBORDER_NONE );

	m_script = new SamScriptCtrl( split, ID_SCRIPT, this );

	m_output = new wxTextCtrl( split, ID_OUTPUT_TEXT, wxT("Ready."), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxBORDER_NONE );
	
	wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( toolbar, 0, wxALL|wxEXPAND, 0 );
	sizer->Add( split, 1, wxALL|wxEXPAND, 0 );
	SetSizer( sizer );
	
	split->SetMinimumPaneSize( 100 );
	split->SplitHorizontally( m_script, m_output, -150 );
	split->SetSashGravity( 1.0 );
		
	std::vector<wxAcceleratorEntry> entries;
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 'n', wxID_NEW ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 'o', wxID_OPEN ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 's', wxID_SAVE ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 'f', wxID_FIND ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 'w', wxID_CLOSE ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_CMD, 'i', wxID_CLOSE ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F1, wxID_HELP ) );
	entries.push_back( wxAcceleratorEntry( wxACCEL_NORMAL, WXK_F5, wxID_EXECUTE ) );
	SetAcceleratorTable( wxAcceleratorTable( entries.size(), &entries[0] ) );

	UpdateWindowTitle();
}


ScriptWindow *ScriptWindow::CreateNewWindow( bool show )
{
	ScriptWindow *sw = new ScriptWindow( SamApp::Window(), wxID_ANY, wxDefaultPosition, wxSize( 760, 800 ) );
	sw->Show( show );
	return sw;
}

void ScriptWindow::OpenFiles( ScriptWindow *current )
{
	wxWindow *parent = current;
	if ( parent == 0 ) parent = SamApp::Window();

	wxFileDialog dlg( parent, "Open script", 
		wxEmptyString, wxEmptyString, "SAM Script Files (*.lk)|*.lk", wxFD_OPEN|wxFD_MULTIPLE );
	if ( wxID_OK == dlg.ShowModal() )
	{
		wxArrayString files;
		dlg.GetPaths( files );
		for( size_t i=0;i<files.size();i++ )
		{
			if ( ScriptWindow *sw = FindOpenFile( files[i] ) )
			{
				sw->Raise();
				sw->SetFocus();
				continue;
			}

			if ( wxFileExists( files[i] ) )
			{
				ScriptWindow *sw = ( current != 0 && current->GetFileName().IsEmpty() ) ? current : CreateNewWindow( false );
				if ( !sw->Load( files[i] ) )
				{
					wxMessageBox( "Failed to load script.\n\n" + files[i] );
					if ( sw != current ) sw->Destroy();
				}
				else
					sw->Show();
			}
		}
	}
}



std::vector<ScriptWindow*> ScriptWindow::GetWindows()
{
	std::vector<ScriptWindow*> list;
	wxFrame *top = SamApp::Window();
	wxWindowList &wl = top->GetChildren();
	for( wxWindowList::iterator it = wl.begin();
		it != wl.end();
		++it )
		if ( ScriptWindow *scrip = dynamic_cast<ScriptWindow*>( *it ) )
			list.push_back( scrip );

	return list;
}

bool ScriptWindow::CloseAll()
{
	bool closed = true;
	std::vector<ScriptWindow*> list = GetWindows();
	for( size_t i=0;i<list.size();i++ )
		if ( !list[i]->Close() )
			closed = false;

	return closed;
}

ScriptWindow *ScriptWindow::FindOpenFile( const wxString &file )
{
	std::vector<ScriptWindow*> list = GetWindows();
	for( size_t i=0;i<list.size();i++ )
		if ( list[i]->GetFileName() == file )
			return list[i];

	return 0;
}

void ScriptWindow::AddOutput( const wxString &out )
{
	m_output->AppendText( out );
}

void ScriptWindow::ClearOutput()
{
	m_output->Clear();
}

bool ScriptWindow::Save()
{
	if ( m_fileName.IsEmpty() )
		return SaveAs();
	else
		return Write( m_fileName );
}

bool ScriptWindow::SaveAs()
{
	wxFileDialog dlg( this, "Save as...",
		wxPathOnly(m_fileName),
		wxFileNameFromPath(m_fileName),
		"SAM Script Files (*.lk)|*.lk", wxFD_SAVE|wxFD_OVERWRITE_PROMPT );
	if (dlg.ShowModal() == wxID_OK)
		return Write( dlg.GetPath() );
	else
		return false;
}

bool ScriptWindow::Load( const wxString &file )
{
	if( m_script->ReadAscii( file ) )
	{
		m_fileName = file;
		UpdateWindowTitle();
		return true;
	}
	else
		return false;
}

bool ScriptWindow::Write( const wxString &file )
{
	wxBusyInfo info( "Saving: " + file, this );
	wxMilliSleep( 120 );

	if ( m_script->WriteAscii( file ) )
	{
		m_fileName = file;
		UpdateWindowTitle();
		return true;
	}
	else return false;
}

void ScriptWindow::UpdateWindowTitle()
{
	wxString title( m_fileName );
	if ( title.IsEmpty() ) title = "untitled";
	if ( m_script->IsModified() ) title += " *";
	if ( m_lastTitle != title )
	{
		SetTitle( title );
		m_lastTitle = title;
	}
}

wxString ScriptWindow::GetFileName()
{
	return m_fileName;
}

void ScriptWindow::OnCommand( wxCommandEvent &evt )
{
	switch( evt.GetId() )
	{
	case wxID_NEW:
		CreateNewWindow();
		break;

	case wxID_OPEN:
		OpenFiles( this );
		break;

	case wxID_SAVE:
		Save();
		break;

	case wxID_SAVEAS:
		SaveAs();
		break;

	case wxID_FIND:
		m_script->ShowFindReplaceDialog();
		break;

	case wxID_EXECUTE:
		m_output->Clear();
		m_runBtn->Hide();
		m_stopBtn->Show();
		Layout();
		wxGetApp().Yield( true );
		m_script->Execute( wxPathOnly(m_fileName), SamApp::Window() );
		m_stopBtn->Hide();
		m_runBtn->Show();
		Layout();
		break;

	case wxID_HELP:
		SamApp::ShowHelp( "scripting" );
		break;
		
	case wxID_STOP:
		// first cancel any simulations that
		// might be running that were invoked 
		// from a script
		ScriptWindow::CancelRunningSimulations();
		m_script->Stop();
		break;

	case wxID_CLOSE:
		Close();
		break;
		
	case ID_FUNCTIONS:
		m_script->ShowHelpDialog();
		break;

	case ID_VARIABLES:
	{
		VarSelectDialog dlg( this, "Browse Variables" );
		if ( Case *c = SamApp::Window()->GetCurrentCase() )
		{
			wxString tech, fin;
			c->GetConfiguration(&tech, &fin);
			dlg.SetConfiguration( tech, fin );
		}

		dlg.CenterOnParent();
		if ( dlg.ShowModal() == wxID_OK )
		{
			m_script->InsertText(
				m_script->GetCurrentPos(), wxJoin(dlg.GetCheckedNames(),',') );
		}
	}
		break;

	};
}

bool ScriptWindow::Find( const wxString &text, 
	bool match_case, bool whole_word, bool at_beginning,
	int *pos, int *line, wxString *line_text )
{
	if ( text.Len() == 0 ) return false;

	int flags = 0;	
	if ( whole_word ) flags |= wxSTC_FIND_WHOLEWORD;	
	if ( match_case ) flags |= wxSTC_FIND_MATCHCASE;

	if ( at_beginning )
		m_lastFindPos = 0;

	m_lastFindPos = m_script->FindText( m_lastFindPos, 
		m_script->GetLength(), text, flags );

	if ( m_lastFindPos >= 0 )
	{
		*pos = m_lastFindPos;
		*line = m_script->LineFromPosition( m_lastFindPos );
		*line_text = m_script->GetLine( *line );
		m_lastFindPos += text.Len();
		return true;
	}
	else
		return false;
}

void ScriptWindow::OnModified( wxStyledTextEvent & )
{
	UpdateWindowTitle();
}

void ScriptWindow::OnClose( wxCloseEvent &evt )
{	
	if ( m_script->IsScriptRunning() )
	{
		if ( wxYES == wxMessageBox("The script is running.  Stop it?", "Query", wxYES_NO, this ) )
			m_script->Stop();

		evt.Veto();
		return;
	}

	if ( m_script->IsModified() )
	{
		Raise();
		int ret = wxMessageBox("The script '" + m_fileName + "' has been modified.  Save changes?", "Query", 
			wxICON_EXCLAMATION|wxYES_NO|wxCANCEL, this );
		if (ret == wxYES)
		{
			Save( );
			if ( m_script->IsModified() ) // if failed to save, cancel
			{
				evt.Veto();
				return;
			}
		}
		else if (ret == wxCANCEL)
		{
			evt.Veto();
			return;
		}
	}
	
	wxGetApp().ScheduleForDestruction( this );
}
