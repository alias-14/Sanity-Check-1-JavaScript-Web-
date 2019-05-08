// Generated code. Do not modify!
//
// Copyright (c) 2001-2019 by PDFTron Systems Inc. All Rights Reserved.
//

//----------------------------------------------------------------------------------------------------------------------
// This sample demonstrates the basic usage of high-level digital signature API in PDFNet.
//
// The following steps are typically used to add a digital signature to a PDF:
//
//     1. Extend and implement a new SignatureHandler. The SignatureHandler will be used to add or validate/check a
//        digital signature.
//     2. Create an instance of the implemented SignatureHandler and register it with PDFDoc with
//        pdfdoc.AddSignatureHandler(). The method returns an ID that can be used later to associate a SignatureHandler
//        with a field.
//     3. Find the required 'e_signature' field in the existing document or create a new field.
//     4. Call field.UseSignatureHandler() with the ID of your handler.
//     5. Call pdfdoc.Save()
//
// Additional processing can be done before document is signed. For example, UseSignatureHandler() returns an instance
// of SDF dictionary which represents the signature dictionary (or the /V entry of the form field). This can be used to
// add additional information to the signature dictionary (e.g. Name, Reason, Location, etc.).
//
// Although the steps above describes extending the SignatureHandler class, this sample demonstrates the use of
// StdSignatureHandler (a built-in SignatureHandler in PDFNet) to sign a PDF file.
//----------------------------------------------------------------------------------------------------------------------

// In order to use .NET Framework's Cryptography library, define "USE_DOTNET_CRYPO" and then add System.Security to
// references list.

using System;
using System.Collections.Generic;
using System.IO;
#if USE_DOTNET_CRYPTO
using System.Security.Cryptography;
using System.Security.Cryptography.Pkcs;
using System.Security.Cryptography.X509Certificates;
#endif // USE_DOTNET_CRYPTO

using pdftron;
using pdftron.Common;
using pdftron.PDF;
using pdftron.PDF.Annots;
using pdftron.SDF;

namespace DigitalSignaturesTestCS
{
#if USE_DOTNET_CRYPTO
    class DotNetCryptoSignatureHandler : SignatureHandler
    {
        private List<byte> m_data;
        private string m_signingCert;
        private string m_certPassword;

        public DotNetCryptoSignatureHandler(string signingCert, string password)
        {
            m_signingCert = signingCert;
            m_certPassword = password;
            m_data = new List<byte>();
        }

        public override void AppendData(byte[] data)
        {
            m_data.AddRange(data);
        }

        public override bool Reset()
        {
            m_data.Clear();
            return (true);
        }

        public override byte[] CreateSignature()
        {
            try {
                ContentInfo ci = new ContentInfo(m_data.ToArray());
                SignedCms sc = new SignedCms(ci, true);
                X509Certificate2 cert = new X509Certificate2(m_signingCert, m_certPassword);
                CmsSigner cs = new CmsSigner();
                cs.Certificate = cert;
                sc.ComputeSignature(cs);
                byte[] sig = sc.Encode();
                return (sig);
            }
            catch (Exception e) {
                Console.Error.WriteLine(e);
            }
            return (null);
        }

        public override string GetName()
        {
            return ("Adobe.PPKLite");
        }
    }
#endif // USE_DOTNET_CRYPTO

    class Class1
	{
        /// <summary>
        /// This functions add an approval signature to the PDF document. The original PDF document contains a blank form field
        /// that is prepared for a user to sign. The following code demonstrate how to sign this document using PDFNet.
        /// </summary>
        /// <returns></returns>
        static bool SignPDF()
        {
            bool result = true;

            string input_path = "../../TestFiles/";
            string output_path = "../../TestFiles/Output/";

            const string input_filename = "doc_to_sign.pdf";
            const string output_filename = "signed_doc.pdf";

            string certfile = "pdftron.pfx";

            // Open an existing PDF
            using (PDFDoc doc = new PDFDoc(input_path + input_filename)) {
                try {
                    Console.Out.WriteLine("Signing PDF document");
#if USE_DOTNET_CRYPTO
                    // Create a new instance of the SignatureHandler.
                    DotNetCryptoSignatureHandler sigHandler = new DotNetCryptoSignatureHandler(input_path + certfile, "password");
                    // Add the SignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
                    SignatureHandlerId sigHandlerId = doc.AddSignatureHandler(sigHandler);
#else // USE_DOTNET_CRYPTO
                    // Add an StdSignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
                    SignatureHandlerId sigHandlerId = doc.AddStdSignatureHandler(input_path + certfile, "password");
#endif // USE_DOTNET_CRYPTO

                    // Obtain the signature form field from the PDFDoc via Annotation;
                    Field sigField = doc.GetField("Signature1");
                    Widget widgetAnnot = new Widget(sigField.GetSDFObj());
                    
                    // Tell PDFNet to use the SignatureHandler created to sign the new signature form field.
                    Obj sigDict = sigField.UseSignatureHandler(sigHandlerId);

                    // Add more information to the signature dictionary
                    sigDict.PutName("SubFilter", "adbe.pkcs7.detached");
                    sigDict.PutString("Name", "PDFTron");
                    sigDict.PutString("Location", "Vancouver, BC");
                    sigDict.PutString("Reason", "Document verification.");

                    // Add the signature appearance
                    ElementWriter apWriter = new ElementWriter();
                    ElementBuilder apBuilder = new ElementBuilder();
                    apWriter.Begin(doc);
                    Image sigImg = Image.Create(doc, input_path + "signature.jpg");
                    double w = sigImg.GetImageWidth(), h = sigImg.GetImageHeight();
                    Element apElement =  apBuilder.CreateImage(sigImg, 0, 0, w, h);
                    apWriter.WritePlacedElement(apElement);
                    Obj apObj = apWriter.End();
                    apObj.PutRect("BBox", 0, 0, w, h);
                    apObj.PutName("Subtype", "Form");
                    apObj.PutName("Type", "XObject");
                    apWriter.Begin(doc);
                    apElement = apBuilder.CreateForm(apObj);
                    apWriter.WritePlacedElement(apElement);
                    apObj = apWriter.End();
                    apObj.PutRect("BBox", 0, 0, w, h);
                    apObj.PutName("Subtype", "Form");
                    apObj.PutName("Type", "XObject");
                    
                    widgetAnnot.SetAppearance(apObj);
                    widgetAnnot.RefreshAppearance();
                    
                    // Save the PDFDoc. Once the method below is called, PDFNet will also sign the document using the information
                    // provided.
                    doc.Save(output_path + output_filename, 0);

                    Console.Out.WriteLine("Finished signing PDF document.");
                }
                catch (Exception e) {
                    Console.Error.WriteLine(e);
                    result = false;
                }
            }
            return (result);
        }

        /// <summary>
        /// Adds a certification signature to the PDF document. Certifying a document is like notarizing a document. Unlike
        /// approval signatures, there can be only one certification per PDF document. Only the first signature in the PDF
        /// document can be used as the certification signature. The process of certifying a document is almost exactly the same
        /// as adding approval signatures with the exception of certification signatures requires an entry in the "Perms"
        /// dictionary.
        /// </summary>
        /// <returns></returns>
        static bool CertifyPDF()
        {
            bool result = true;

            string input_path = "../../TestFiles/";
            string output_path = "../../TestFiles/Output/";

            const string input_filename = "newsletter.pdf";
            const string output_filename = "newsletter_certified.pdf";

            string certfile = "pdftron.pfx";

            // Open an existing PDF
            using (PDFDoc doc = new PDFDoc(input_path + input_filename)) {
                try {
                    Console.Out.WriteLine("Certifying PDF document");
#if USE_DOTNET_CRYPTO
                    // Create a new instance of the SignatureHandler.
                    DotNetCryptoSignatureHandler sigHandler = new DotNetCryptoSignatureHandler(input_path + certfile, "password");
                    // Add the SignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
                    SignatureHandlerId sigHandlerId = doc.AddSignatureHandler(sigHandler);
#else // USE_DOTNET_CRYPTO
                    // Add an StdSignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
                    SignatureHandlerId sigHandlerId = doc.AddStdSignatureHandler(input_path + certfile, "password");
#endif // USE_DOTNET_CRYPTO

                    // Create new signature form field in the PDFDoc.
                    Field sigField = doc.FieldCreate("Signature1", Field.Type.e_signature);

                    Page page1 = doc.GetPage(1);
                    Widget widgetAnnot = Widget.Create(doc.GetSDFDoc(), new Rect(0, 0, 0, 0), sigField);
                    page1.AnnotPushBack(widgetAnnot);
                    widgetAnnot.SetPage(page1);
                    Obj widgetObj = widgetAnnot.GetSDFObj();
                    widgetObj.PutNumber("F", 132);
                    widgetObj.PutName("Type", "Annot");

                    // Tell PDFNet to use the SignatureHandler created to sign the new signature form field
                    Obj sigDict = sigField.UseSignatureHandler(sigHandlerId);

                    // Add more information to the signature dictionary
                    sigDict.PutName("SubFilter", "adbe.pkcs7.detached");
                    sigDict.PutString("Name", "PDFTron");
                    sigDict.PutString("Location", "Vancouver, BC");
                    sigDict.PutString("Reason", "Document verification.");

                    // Appearance can be added to the widget annotation. Please see the "SignPDF()" function for details.

                    // Add this sigDict as DocMDP in Perms dictionary from root
                    Obj root = doc.GetRoot();
                    Obj perms = root.PutDict("Perms");
                    // add the sigDict as DocMDP (indirect) in Perms
                    perms.Put("DocMDP", sigDict);
                    
                    // add the additional DocMDP transform params
                    Obj refObj = sigDict.PutArray("Reference");
                    Obj transform = refObj.PushBackDict();
                    transform.PutName("TransformMethod", "DocMDP");
                    transform.PutName("Type", "SigRef");
                    Obj transformParams = transform.PutDict("TransformParams");
                    transformParams.PutNumber("P", 1); // Set permissions as necessary.
                    transformParams.PutName("Type", "TransformParams");
                    transformParams.PutName("V", "1.2");

                    // Save the PDFDoc. Once the method below is called, PDFNet will also sign the document using the information
                    // provided.
                    doc.Save(output_path + output_filename, 0);

                    Console.Out.WriteLine("Finished certifying PDF document.");
                }
                catch (Exception e) {
                    Console.Error.WriteLine(e);
                    result = false;
                }
            }
            return (result);
        }

        private static pdftron.PDFNetLoader pdfNetLoader = pdftron.PDFNetLoader.Instance();
        static Class1() {}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            // Initialize PDFNet
            PDFNet.Initialize();

            bool result = true;

            if (!SignPDF())
                result = false;

            if (!CertifyPDF())
                result = false;

            Console.Out.WriteLine();

            if (!result) {
                Console.Out.WriteLine("Tests failed.");
                return;
            }

            Console.Out.WriteLine("All tests passed.");
        }
	}
}
