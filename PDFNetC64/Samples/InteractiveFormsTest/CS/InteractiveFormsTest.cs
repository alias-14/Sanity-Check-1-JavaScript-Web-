// Generated code. Do not modify!
//---------------------------------------------------------------------------------------
// Copyright (c) 2001-2019 by PDFTron Systems Inc. All Rights Reserved.
// Consult legal.txt regarding legal and license information.
//---------------------------------------------------------------------------------------

using System;
using pdftron;
using pdftron.Common;
using pdftron.Filters;
using pdftron.SDF;
using pdftron.PDF;

namespace FormsTestCS
{
	/// <summary>
	///---------------------------------------------------------------------------------------
	/// This sample illustrates basic PDFNet capabilities related to interactive 
	/// forms (also known as AcroForms). 
	///---------------------------------------------------------------------------------------
	/// </summary>
	class Class1
	{
		private static pdftron.PDFNetLoader pdfNetLoader = pdftron.PDFNetLoader.Instance();
		static Class1() {}
		
		static void Main(string[] args)
		{
			PDFNet.Initialize();

			// Relative path to the folder containing test files.
			// string input_path =  "../../TestFiles/";
			string output_path = "../../TestFiles/Output/";

			//----------------------------------------------------------------------------------
			// Example 1: Programatically create new Form Fields and Widget Annotations.
			//----------------------------------------------------------------------------------
			try
			{
				using (PDFDoc doc = new PDFDoc())
				{
					Page blank_page = doc.PageCreate();	// Create a blank page and some new fields (aka "AcroForms").

					// Create new fields.
					Field emp_first_name = doc.FieldCreate("employee.name.first", 
						Field.Type.e_text, doc.CreateIndirectString("John"));
					Field emp_last_name = doc.FieldCreate("employee.name.last", 
						Field.Type.e_text, doc.CreateIndirectString("Doe"));
					Field emp_last_check1 = doc.FieldCreate("employee.name.check1", 
						Field.Type.e_check, doc.CreateIndirectName("Yes"));
					Field submit = doc.FieldCreate("submit", 
						Field.Type.e_button);

					// Create page annotations for the above fields.
					SDFDoc sdfdoc = doc.GetSDFDoc();

					// Create text annotations
					pdftron.PDF.Annots.Widget annot1 = pdftron.PDF.Annots.Widget.Create(sdfdoc, new Rect(50, 550, 350, 600), emp_first_name);
							pdftron.PDF.Annots.Widget annot2 = pdftron.PDF.Annots.Widget.Create(sdfdoc, new Rect(50, 450, 350, 500), emp_last_name);

					// Create check-box annotation
							pdftron.PDF.Annots.Widget annot3 = pdftron.PDF.Annots.Widget.Create(doc, new Rect(64, 356, 120, 410), emp_last_check1);
					// Set the annotation appearance for the "Yes" state...
					annot3.SetAppearance(CreateCheckmarkAppearance(doc), Annot.AnnotationState.e_normal, "Yes");
					
					// Create button annotation
							pdftron.PDF.Annots.Widget annot4 = pdftron.PDF.Annots.Widget.Create(doc, new Rect(64, 284, 163, 320), submit);
					// Set the annotation appearances for the down and up state...
					annot4.SetAppearance(CreateButtonAppearance(doc, false), Annot.AnnotationState.e_normal);
					annot4.SetAppearance(CreateButtonAppearance(doc, true), Annot.AnnotationState.e_down);
					
					// Create 'SubmitForm' action. The action will be linked to the button.
					FileSpec url = FileSpec.CreateURL(doc, "http://www.pdftron.com");
					pdftron.PDF.Action button_action = pdftron.PDF.Action.CreateSubmitForm(url);

					// Associate the above action with 'Down' event in annotations action dictionary.
					Obj annot_action = 	annot4.GetSDFObj().PutDict("AA");
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
					doc.RefreshFieldAppearances();				

					doc.Save(output_path + "forms_test1.pdf", 0);

					Console.WriteLine("Done.");
				}
			}
			catch (PDFNetException e)
			{
				Console.WriteLine(e.Message);
			}

			//----------------------------------------------------------------------------------
			// Example 2: 
			// Fill-in forms / Modify values of existing fields.
			// Traverse all form fields in the document (and print out their names). 
			// Search for specific fields in the document.
			//----------------------------------------------------------------------------------
			try  
			{
				using (PDFDoc doc = new PDFDoc(output_path + "forms_test1.pdf"))
				{
					doc.InitSecurityHandler();

					FieldIterator itr;
					for(itr=doc.GetFieldIterator(); itr.HasNext(); itr.Next()) 
					{
						Field field = itr.Current();
						Console.WriteLine("Field name: {0}", field.GetName());
						Console.WriteLine("Field partial name: {0}", field.GetPartialName());
                        string str_val = field.GetValueAsString();

						Console.Write("Field type: ");
						Field.Type type = field.GetType();
						switch(type)
						{
						case Field.Type.e_button: 
							Console.WriteLine("Button");
							break;
						case Field.Type.e_radio: 
							Console.WriteLine("Radio button: Value = " + str_val);
							break;
						case Field.Type.e_check: 
							field.SetValue(true);
							Console.WriteLine("Check box: Value = " + str_val);
							break;
						case Field.Type.e_text:
							{
								Console.WriteLine("Text"); 

								// Edit all variable text in the document
								String old_value = "none";
								if (field.GetValue() != null)
									old_value = field.GetValue().GetAsPDFText();

								field.SetValue("This is a new value. The old one was: " + old_value);
							}
							break;
						case Field.Type.e_choice:
							Console.WriteLine("Choice"); 
							break;
						case Field.Type.e_signature:
							Console.WriteLine("Signature"); 
							break;
						}

						Console.WriteLine("------------------------------");
					}

					// Search for a specific field
					Field fld = doc.GetField("employee.name.first");
					if (fld != null) 
					{
						Console.WriteLine("Field search for {0} was successful", fld.GetName());
					}
					else 
					{
						Console.WriteLine("Field search failed.");
					}

					// Regenerate field appearances.
					doc.RefreshFieldAppearances();
					doc.Save(output_path + "forms_test_edit.pdf", 0);
					Console.WriteLine("Done.");
				}
			}
			catch (PDFNetException e)
			{
				Console.WriteLine(e.Message);
			}

			//----------------------------------------------------------------------------------
			// Sample: Form templating
			// Replicate pages and form data within a document. Then rename field names to make 
			// them unique.
			//----------------------------------------------------------------------------------
			try  
			{
				// Sample: Copying the page with forms within the same document
				using (PDFDoc doc = new PDFDoc(output_path + "forms_test1.pdf"))
				{
					doc.InitSecurityHandler();

					Page src_page = doc.GetPage(1);
					doc.PagePushBack(src_page);  // Append several copies of the second page
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

					doc.Save(output_path + "forms_test1_cloned.pdf", 0);
					Console.WriteLine("Done.");
				}
			}
			catch (PDFNetException e)
			{
				Console.WriteLine(e.Message);
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
				using (PDFDoc doc = new PDFDoc(output_path + "forms_test1.pdf"))
				{
					doc.InitSecurityHandler();

					bool auto = true;
					if (auto)
					{
						doc.FlattenAnnotations();
					}
					else  // Manual flattening 
					{
						// Traverse all pages
						PageIterator pitr = doc.GetPageIterator();
						for (; pitr.HasNext(); pitr.Next())
						{
							Page page = pitr.Current();
							Obj annots = page.GetAnnots();
							if (annots != null)
							{	// Look for all widget annotations (in reverse order)
								for (int i = annots.Size()-1; i>=0; --i)
								{
									if (annots.GetAt(i).Get("Subtype")
										.Value().GetName() == "Widget")
									{
										Field field = new Field(annots.GetAt(i));
										field.Flatten(page);

										// Another way of making a read only field is by modifying 
										// field's e_read_only flag: 
										//  field.SetFlag(Field.FieldFlag.e_read_only, true);
									}
								}
							}
						}
					}

					doc.Save(output_path + "forms_test1_flattened.pdf", 0);
					Console.WriteLine("Done.");
				}
			}
			catch (PDFNetException e)
			{
				Console.WriteLine(e.Message);
			}


		}

		static void RenameAllFields(PDFDoc doc, String name)
		{
			Field fld = doc.GetField(name);
			for (int counter=0; fld!=null; ++counter)
			{
				fld.Rename(name + counter.ToString());
				fld = doc.GetField(name);
			}
		}

		static Obj CreateCheckmarkAppearance(PDFDoc doc) 
		{
			// Create a checkmark appearance stream ------------------------------------
			using (ElementBuilder builder = new ElementBuilder())
			using (ElementWriter writer = new ElementWriter())
			{
				writer.Begin(doc); 
				writer.WriteElement(builder.CreateTextBegin()); 
		
				// other options are circle ("l"), diamond ("H"), cross ("\x35")
				// See section D.4 "ZapfDingbats Set and Encoding" in PDF Reference Manual for 
				// the complete graphical map for ZapfDingbats font.
				Element checkmark = builder.CreateTextRun("4", Font.Create(doc, Font.StandardType1Font.e_zapf_dingbats), 1);
				writer.WriteElement(checkmark);
	
				writer.WriteElement(builder.CreateTextEnd());

				Obj stm = writer.End();

				// Set the bounding box
				stm.PutRect("BBox", -0.2, -0.2, 1, 1);
				stm.PutName("Subtype", "Form");
				return stm;
			}
		}

		static Obj CreateButtonAppearance(PDFDoc doc, bool button_down) 
		{
			// Create a button appearance stream ------------------------------------
			using (ElementBuilder builder = new ElementBuilder())
			using (ElementWriter writer = new ElementWriter())
			{
				writer.Begin(doc); 

				// Draw background
				Element element = builder.CreateRect(0, 0, 101, 37);
				element.SetPathFill(true);
				element.SetPathStroke(false);
				element.GetGState().SetFillColorSpace(ColorSpace.CreateDeviceGray());
				element.GetGState().SetFillColor(new ColorPt(0.75, 0.0, 0.0));
				writer.WriteElement(element); 

				// Draw 'Submit' text
				writer.WriteElement(builder.CreateTextBegin()); 
		
				element = builder.CreateTextRun("Submit", Font.Create(doc, Font.StandardType1Font.e_helvetica_bold), 12);
				element.GetGState().SetFillColor(new ColorPt(0, 0, 0));

				if (button_down) 
					element.SetTextMatrix(1, 0, 0, 1, 33, 10);
				else 
					element.SetTextMatrix(1, 0, 0, 1, 30, 13);
				writer.WriteElement(element);
				writer.WriteElement(builder.CreateTextEnd());

				Obj stm = writer.End();

				// Set the bounding box
				stm.PutRect("BBox", 0, 0, 101, 37);
				stm.PutName("Subtype", "Form");
				return stm;
			}
		}
	}
}
