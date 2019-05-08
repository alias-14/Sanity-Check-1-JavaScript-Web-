//---------------------------------------------------------------------------------------
// Copyright (c) 2001-2019 by PDFTron Systems Inc. All Rights Reserved.
// Consult legal.txt regarding legal and license information.
//---------------------------------------------------------------------------------------

#include <PDF/PDFNet.h>
#include <PDF/PDFDoc.h>
#include <PDF/Annot.h>
#include <PDF/Field.h>
#include <PDF/ElementBuilder.h>
#include <PDF/ElementWriter.h>

#include <iostream>

using namespace std;
using namespace pdftron;
using namespace SDF;
using namespace PDF;


//---------------------------------------------------------------------------------------
// This sample illustrates basic PDFNet capabilities related to interactive 
// forms (also known as AcroForms). 
//---------------------------------------------------------------------------------------

void RenameAllFields(PDFDoc& doc, const UString& name)
{
	char tmp[32];
	FieldIterator itr = doc.GetFieldIterator(name);
	for (int counter=0; itr.HasNext();
		itr=doc.GetFieldIterator(name), ++counter)
	{
		Field f = itr.Current();
		sprintf(tmp, "%d", counter);
		f.Rename(name + tmp);		
	}
}

enum CheckStyle {
	e_check,
	e_circle,
	e_cross,
	e_diamond,
	e_square,
	e_star
}; 

// Note: The visual appearance of check-marks and radio-buttons in PDF documents is 
// not limited to CheckStyle-s. It is possible to create a visual appearance using 
// arbitrary glyph, text, raster image, or path object. Although most PDF producers 
// limit the options to the above 'standard' styles, using PDFNet you can generate 
// arbitrary appearances.
Obj CreateCheckmarkAppearance(PDFDoc& doc, CheckStyle style) 
{
	// Create a checkmark appearance stream ------------------------------------
	ElementBuilder build;
	ElementWriter writer;
	writer.Begin(doc);
	writer.WriteElement(build.CreateTextBegin());

	char symbol;
	switch (style) {
		case e_circle: symbol = '\154'; break;
		case e_diamond: symbol = '\165'; break;
		case e_cross: symbol = '\65'; break;
		case e_square: symbol = '\156'; break;
		case e_star: symbol = '\110'; break;
			// ...
			// See section D.4 "ZapfDingbats Set and Encoding" in PDF Reference Manual 
			// (http://www.pdftron.com/downloads/PDFReference16.pdf) for the complete 
			// graphical map for ZapfDingbats font. Please note that all character codes
			// are represented using the 'octal' notation.
		default:  // e_check
			symbol = '\64';
	}

	Element checkmark = build.CreateTextRun(&symbol, 1, Font::Create(doc, PDF::Font::e_zapf_dingbats), 1);
	writer.WriteElement(checkmark);
	writer.WriteElement(build.CreateTextEnd());

	Obj stm = writer.End();
	stm.PutRect("BBox", -0.2, -0.2, 1, 1); // Clip
	stm.PutName("Subtype", "Form");
	return stm;
}

Obj CreateButtonAppearance(PDFDoc& doc, bool button_down) 
{
	// Create a button appearance stream ------------------------------------
	ElementBuilder build;
	ElementWriter writer;
	writer.Begin(doc); 

	// Draw background
	Element element = build.CreateRect(0, 0, 101, 37);
	element.SetPathFill(true);
	element.SetPathStroke(false);
	element.GetGState().SetFillColorSpace(ColorSpace::CreateDeviceGray());
	element.GetGState().SetFillColor(ColorPt(0.75));
	writer.WriteElement(element); 

	// Draw 'Submit' text
	writer.WriteElement(build.CreateTextBegin()); 
	{
		const char* text = "Submit";
		element = build.CreateTextRun(text, UInt32(strlen(text)), Font::Create(doc, PDF::Font::e_helvetica_bold), 12);
		element.GetGState().SetFillColor(ColorPt(0));

		if (button_down) 
			element.SetTextMatrix(1, 0, 0, 1, 33, 10);
		else 
			element.SetTextMatrix(1, 0, 0, 1, 30, 13);
		writer.WriteElement(element);
	}
	writer.WriteElement(build.CreateTextEnd());

	Obj stm = writer.End(); 

	// Set the bounding box
	stm.PutRect("BBox", 0, 0, 101, 37);
	stm.PutName("Subtype","Form");
	return stm;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	PDFNet::Initialize();

	// Relative path to the folder containing test files.
	// string input_path =  "../../TestFiles/";
	string output_path = "../../TestFiles/Output/";

	//----------------------------------------------------------------------------------
	// Example 1: Programatically create new Form Fields and Widget Annotations.
	//----------------------------------------------------------------------------------
	try  
	{
		PDFDoc doc;		
		Page blank_page = doc.PageCreate(); // Create a blank new page and add some form fields.

		// Create new fields.
		Field emp_first_name = doc.FieldCreate("employee.name.first", Field::e_text, "John", "");
		Field emp_last_name = doc.FieldCreate("employee.name.last", Field::e_text, "Doe", "");
		Field emp_last_check1 = doc.FieldCreate("employee.name.check1", Field::e_check, "Yes", "");

		Field submit = doc.FieldCreate("submit", Field::e_button);

		// Create page annotations for the above fields.

		// Create text annotations
		Annots::Widget annot1 = Annots::Widget::Create(doc, Rect(50, 550, 350, 600), emp_first_name);
		Annots::Widget annot2 = Annots::Widget::Create(doc, Rect(50, 450, 350, 500), emp_last_name);

		// Create a check-box annotation
		Annots::Widget annot3 = Annots::Widget::Create(doc, Rect(64, 356, 120, 410), emp_last_check1);
		// Set the annotation appearance for the "Yes" state...
		// NOTE: if we call RefreshFieldAppearances after this the appearance will be discarded
		annot3.SetAppearance(CreateCheckmarkAppearance(doc, e_check), Annot::e_normal, "Yes");
		
		// Create button annotation
		Annots::Widget annot4 = Annots::Widget::Create(doc, Rect(64, 284, 163, 320), submit);
		// Set the annotation appearances for the down and up state...
		annot4.SetAppearance(CreateButtonAppearance(doc, false), Annot::e_normal);
		annot4.SetAppearance(CreateButtonAppearance(doc, true), Annot::e_down);
		
		// Create 'SubmitForm' action. The action will be linked to the button.
		FileSpec url = FileSpec::CreateURL(doc, "http://www.pdftron.com");
		Action button_action = Action::CreateSubmitForm(url);

		// Associate the above action with 'Down' event in annotations action dictionary.
		Obj annot_action = annot4.GetSDFObj().PutDict("AA");
		annot_action.Put("D", button_action.GetSDFObj());

		blank_page.AnnotPushBack(annot1);  // Add annotations to the page
		blank_page.AnnotPushBack(annot2);
		blank_page.AnnotPushBack(annot3);
		blank_page.AnnotPushBack(annot4);

		doc.PagePushBack(blank_page);	// Add the page as the last page in the document.

		// If you are not satisfied with the look of default auto-generated appearance 
		// streams you can delete "AP" entry from the Widget annotation and set 
		// "NeedAppearances" flag in AcroForm dictionary:
		//    doc.GetAcroForm().PutBool("NeedAppearances", true);
		// This will force the viewer application to auto-generate new appearance streams 
		// every time the document is opened.
		//
		// Alternatively you can generate custom annotation appearance using ElementWriter 
		// and then set the "AP" entry in the widget dictionary to the new appearance
		// stream.
		//
		// Yet another option is to pre-populate field entries with dummy text. When 
		// you edit the field values using PDFNet the new field appearances will match 
		// the old ones.

		//doc.GetAcroForm().PutBool("NeedAppearances", true);
		// NOTE: RefreshFieldAppearances will replace previously generated appearance streams
		doc.RefreshFieldAppearances();

		doc.Save((output_path + "forms_test1.pdf").c_str(), 0, 0);
		cout << "Done." << endl;
	}
	catch(Common::Exception& e)
	{
		cout << e << endl;
		ret = 1;
	}
	catch(...)
	{
		cout << "Unknown Exception" << endl;
		ret = 1;
	}

	//----------------------------------------------------------------------------------
	// Example 2: 
	// Fill-in forms / Modify values of existing fields.
	// Traverse all form fields in the document (and print out their names). 
	// Search for specific fields in the document.
	//----------------------------------------------------------------------------------
	try  
	{
		PDFDoc doc((output_path + "forms_test1.pdf").c_str());
		doc.InitSecurityHandler();

		FieldIterator itr = doc.GetFieldIterator();
		for(; itr.HasNext(); itr.Next()) 
		{
			cout << "Field name: " << itr.Current().GetName() << endl;
			cout << "Field partial name: " << itr.Current().GetPartialName() << endl;

			cout << "Field type: ";
			Field::Type type = itr.Current().GetType();
			UString str_val = itr.Current().GetValueAsString();

			switch(type)
			{
			case Field::e_button: 
				cout << "Button" << endl; 
				break;
			case Field::e_radio: 
				cout << "Radio button: Value = " << str_val << endl; 
				break;
			case Field::e_check: 
				itr.Current().SetValue(true);
				cout << "Check box: Value = " << str_val << endl; 
				break;
			case Field::e_text: 
				{
					cout << "Text" << endl;
					// Edit all variable text in the document
					itr.Current().SetValue(UString("This is a new value. The old one was: ") + str_val);
				}
				break;
			case Field::e_choice: cout << "Choice" << endl; break;
			case Field::e_signature: cout << "Signature" << endl; break;
			}

			cout << "------------------------------" << endl;
		}

		// Search for a specific field
		Field f = doc.GetField("employee.name.first");
		if (f) 
		{
			cout << "Field search for " << f.GetName() << " was successful" << endl;
		}
		else 
		{
			cout << "Field search failed" << endl;
		}

		// Regenerate field appearances.
		doc.RefreshFieldAppearances();
		doc.Save((output_path + "forms_test_edit.pdf").c_str(), 0, NULL);
		cout << "Done." << endl;
	}
	catch(Common::Exception& e)
	{
		cout << e << endl;
		ret = 1;
	}
	catch(...)
	{
		cout << "Unknown Exception" << endl;
		ret = 1;
	}

	//----------------------------------------------------------------------------------
	// Sample: Form templating
	// Replicate pages and form data within a document. Then rename field names to make 
	// them unique.
	//----------------------------------------------------------------------------------
	try  
	{
		// Sample: Copying the page with forms within the same document
		PDFDoc doc((output_path + "forms_test1.pdf").c_str());
		doc.InitSecurityHandler();

		Page src_page = doc.GetPage(1);
		doc.PagePushBack(src_page);  // Append several copies of the first page
		doc.PagePushBack(src_page);	 // Note that forms are successfully copied
		doc.PagePushBack(src_page);
		doc.PagePushBack(src_page);

		// Now we rename fields in order to make every field unique.
		// You can use this technique for dynamic template filling where you have a 'master'
		// form page that should be replicated, but with unique field names on every page. 
		RenameAllFields(doc, "employee.name.first");
		RenameAllFields(doc, "employee.name.last");
		RenameAllFields(doc, "employee.name.check1");
		RenameAllFields(doc, "submit");

		doc.Save((output_path + "forms_test1_cloned.pdf").c_str(), 0, 0);
		cout << "Done." << endl;
	}
	catch(Common::Exception& e)
	{
		cout << e << endl;
		ret = 1;
	}
	catch(...)
	{
		cout << "Unknown Exception" << endl;
		ret = 1;
	}

	//----------------------------------------------------------------------------------
	// Sample: 
	// Flatten all form fields in a document.
	// Note that this sample is intended to show that it is possible to flatten
	// individual fields. PDFNet provides a utility function PDFDoc.FlattenAnnotations()
	// that will automatically flatten all fields.
	//----------------------------------------------------------------------------------
	try  
	{
		PDFDoc doc((output_path + "forms_test1.pdf").c_str());
		doc.InitSecurityHandler();

		// Traverse all pages
		if (true) {
			doc.FlattenAnnotations();
		}
		else // Manual flattening
		{			
			
			for (PageIterator pitr = doc.GetPageIterator(); 
				pitr.HasNext(); pitr.Next())  
			{
				Page page = pitr.Current();
				Obj annots = page.GetAnnots();
				if (annots)
				{	// Look for all widget annotations (in reverse order)
					for (int i = int(annots.Size())-1; i>=0; --i)
					{
						if (!strcmp(annots.GetAt(i).Get("Subtype").Value().GetName(), "Widget"))
						{
							Field field(annots.GetAt(i));
							field.Flatten(page);

							// Another way of making a read only field is by modifying 
							// field's e_read_only flag: 
							//    field.SetFlag(Field::e_read_only, true);
						}
					}
				}
			}
		}

		doc.Save((output_path + "forms_test1_flattened.pdf").c_str(), 0, 0);
		cout << "Done." << endl;
	}
	catch(Common::Exception& e)
	{
		cout << e << endl;
		ret = 1;
	}
	catch(...)
	{
		cout << "Unknown Exception" << endl;
		ret = 1;
	}

	PDFNet::Terminate();
	return ret;
}


