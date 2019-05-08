//---------------------------------------------------------------------------------------
// Copyright (c) 2001-2019 by PDFTron Systems Inc. All Rights Reserved.
// Consult legal.txt regarding legal and license information.
//---------------------------------------------------------------------------------------

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

// To build and run this sample with OpenSSL, please specify OpenSSL include & lib paths to project settings.
//
// In MSVC, this can be done by opening the DigitalSignatureTest project's properties. Go to Configuration Properties ->
// C/C++ -> General -> Additional Include Directories. Add the path to the OpenSSL headers here. Next, go to
// Configuration Properties -> Linker -> General -> Additional Library Directories. Add the path to the OpenSSL libraries
// here. Finally, under Configuration Properties -> Linker -> Input -> Additional Dependencies, add libeay32.lib,
// crypt32.lib, and advapi32.lib in the list.
//
// For GCC, modify the Makefile, add -lcrypto to the $(LIBS) variable. If OpenSSL is installed elsewhere, it may be
// necessary to add the path to the headers in the $(INCLUDE) variable as well as the location of either libcrypto.a or
// libcrypto.so/libcrypto.dylib.
//

#define USE_STD_SIGNATURE_HANDLER 1 // Comment this line if using the OpenSSLSignatureHandler.

// standard library includes
#include <cstdio>
#include <iostream>
#include <vector>

// PDFNetC includes
#include <Common/Exception.h>
#include <Common/UString.h>
#include <PDF/Page.h>
#include <PDF/Annot.h>
#include <PDF/Annots/Widget.h>
#include <PDF/Date.h>
#include <PDF/Element.h>
#include <PDF/ElementBuilder.h>
#include <PDF/ElementWriter.h>
#include <PDF/Field.h>
#include <PDF/Image.h>
#include <PDF/PDFDoc.h>
#include <PDF/PDFNet.h>
#include <SDF/SignatureHandler.h>

#if (!USE_STD_SIGNATURE_HANDLER)
// OpenSSL includes
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/pkcs7.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#endif // (!USE_STD_SIGNATURE_HANDLER)

using namespace std;
using namespace pdftron;
using namespace pdftron::SDF;
using namespace pdftron::PDF::Annots;
using namespace pdftron::PDF;

#if (!USE_STD_SIGNATURE_HANDLER)
//
// Extend SignatureHandler by using OpenSSL signing utilities.
//
class OpenSSLSignatureHandler : public SignatureHandler
{
public:
    OpenSSLSignatureHandler(const char* in_pfxfile, const char* in_password) : m_pfxfile(in_pfxfile), m_password(in_password)
    {
        FILE* fp = fopen(in_pfxfile, "rb");
        if (fp == NULL)
            throw (Common::Exception("Cannot open private key.", __LINE__, __FILE__, "PKCS7Signature::PKCS7Signature", "Cannot open private key."));

        PKCS12* p12 = d2i_PKCS12_fp(fp, NULL);
        fclose(fp);

        if (p12 == NULL)
            throw (Common::Exception("Cannot parse private key.", __LINE__, __FILE__, "PKCS7Signature::PKCS7Signature", "Cannot parse private key."));

        mp_pkey = NULL;
        mp_x509 = NULL;
        mp_ca = NULL;
        int parseResult = PKCS12_parse(p12, in_password, &mp_pkey, &mp_x509, &mp_ca);
        PKCS12_free(p12);

        if (parseResult == 0)
            throw (Common::Exception("Cannot parse private key.", __LINE__, __FILE__, "PKCS7Signature::PKCS7Signature", "Cannot parse private key."));

        Reset();
    }

    virtual UString GetName() const
    {
        return (UString("Adobe.PPKLite"));
    }

    virtual void AppendData(const std::vector<pdftron::UInt8>& in_data)
    {
        SHA1_Update(&m_sha_ctx, (const void*) &(in_data[0]), in_data.size());
        return;
    }

    virtual bool Reset()
    {
        m_digest.resize(0);
        m_digest.clear();
        SHA1_Init(&m_sha_ctx);
        return (true);
    }

    virtual std::vector<pdftron::UInt8> CreateSignature()
    {
        if (m_digest.size() == 0) {
            m_digest.resize(SHA_DIGEST_LENGTH);
            SHA1_Final(&(m_digest[0]), &m_sha_ctx);
        }

        PKCS7* p7 = PKCS7_new();
        PKCS7_set_type(p7, NID_pkcs7_signed);

        PKCS7_SIGNER_INFO* p7Si = PKCS7_add_signature(p7, mp_x509, mp_pkey, EVP_sha1());
        PKCS7_add_attrib_content_type(p7Si, OBJ_nid2obj(NID_pkcs7_data));
        PKCS7_add0_attrib_signing_time(p7Si, NULL);
        PKCS7_add1_attrib_digest(p7Si, &(m_digest[0]), m_digest.size());
        PKCS7_add_certificate(p7, mp_x509);    

        for (int c = 0; c < sk_X509_num(mp_ca); c++) {
            X509* cert = sk_X509_value(mp_ca, c);
            PKCS7_add_certificate(p7, cert);
        }
        PKCS7_set_detached(p7, 1);
        PKCS7_content_new(p7, NID_pkcs7_data);

        PKCS7_SIGNER_INFO_sign(p7Si);

        int p7Len = i2d_PKCS7(p7, NULL);
        std::vector<unsigned char> result(p7Len);
        UInt8* pP7Buf = &(result[0]);
        i2d_PKCS7(p7, &pP7Buf);

        PKCS7_free(p7);

        return (result);
    }

    virtual OpenSSLSignatureHandler* Clone() const
    {
        return (new OpenSSLSignatureHandler(m_pfxfile.c_str(), m_password.c_str()));
    }

    virtual ~OpenSSLSignatureHandler()
    {
        sk_X509_free(mp_ca);
        X509_free(mp_x509);
        EVP_PKEY_free(mp_pkey);
    }

private:
    std::vector<UInt8> m_digest;
    std::string m_pfxfile;
    std::string m_password;

    SHA_CTX m_sha_ctx;
    EVP_PKEY* mp_pkey;      // private key
    X509* mp_x509;          // signing certificate
    STACK_OF(X509)* mp_ca;  // certificate chain up to the CA
}; // class OpenSSLSignatureHandler
#endif // (!USE_STD_SIGNATURE_HANDLER)

//
// This functions add an approval signature to the PDF document. The original PDF document contains a blank form field
// that is prepared for a user to sign. The following code demonstrate how to sign this document using PDFNet.
//
bool SignPDF(void)
{
    UString infile("../../TestFiles/doc_to_sign.pdf");
    UString outfile("../../TestFiles/Output/signed_doc.pdf");
    UString certfile("../../TestFiles/pdftron.pfx");
    UString imagefile("../../TestFiles/signature.jpg");
    bool result = true;

    try {
        // Open an existing PDF
        PDFDoc doc(infile);
		cout << "Signing PDF document" << endl;

        // Add an StdSignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
        SDF::SignatureHandlerId sigHandlerId = doc.AddStdSignatureHandler(certfile, UString("password"));
        // When using OpenSSLSignatureHandler class, uncomment the following lines and comment the line above.
        // Create a new instance of the SignatureHandler.
        //OpenSSLSignatureHandler sigHandler(certfile.ConvertToUtf8().c_str(), "password");
        // Add the SignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
        //SDF::SignatureHandlerId sigHandlerId = doc.AddSignatureHandler(sigHandler);
        
        // Obtain the signature form field from the PDFDoc via Annotation.
        Field sigField = doc.GetField(UString("Signature1"));
        Widget widgetAnnot(sigField.GetSDFObj());
        
        // Tell PDFNetC to use the SignatureHandler created to sign the new signature form field.
        SDF::Obj sigDict = sigField.UseSignatureHandler(sigHandlerId);
        
        // Add more information to the signature dictionary.
        //sigDict.PutName("SubFilter", "adbe.pkcs7.detached");
        sigDict.PutString("Name", "PDFTron");
        sigDict.PutString("Location", "Vancouver, BC");
        sigDict.PutString("Reason", "Document verification.");

        // Add the signature appearance.
        ElementWriter apWriter;
        ElementBuilder apBuilder;
        apWriter.Begin(doc);
        Image sigImg = Image::Create(doc, imagefile);
        double w = sigImg.GetImageWidth(), h = sigImg.GetImageHeight();
        Element apElement =  apBuilder.CreateImage(sigImg, 0, 0, w, h);
        apWriter.WritePlacedElement(apElement);
        SDF::Obj apObj = apWriter.End();
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
        
        // Save the PDFDoc. Once the method below is called, PDFNetC will also sign the document using the information
        // provided.
        doc.Save(outfile, 0, NULL);

        cout << "Finished signing PDF document.\n";
    }
    catch (Common::Exception& e) {
        cerr << e << "\n";
        result = false;
    }
    catch (exception& e) {
        cerr << e.what() << "\n";
        result = false;
    }
    catch (...) {
        cerr << "Unknown exception.\n";
        result = false;
    }

    return (result);
}

//
// Adds a certification signature to the PDF document. Certifying a document is like notarizing a document. Unlike
// approval signatures, there can be only one certification per PDF document. Only the first signature in the PDF
// document can be used as the certification signature. The process of certifying a document is almost exactly the same
// as adding approval signatures with the exception of certification signatures requires an entry in the "Perms"
// dictionary.
//
bool CertifyPDF(void)
{
    UString infile("../../TestFiles/newsletter.pdf");
    UString outfile("../../TestFiles/Output/newsletter_certified.pdf");
    UString certfile("../../TestFiles/pdftron.pfx");
    bool result = true;

    try {
        // Open an existing PDF
        PDFDoc doc(infile);
		cout << "Certifying PDF document" << endl;

        // Add an StdSignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
        SDF::SignatureHandlerId sigHandlerId = doc.AddStdSignatureHandler(certfile, UString("password"));
        // When using OpenSSLSignatureHandler class, uncomment the following lines and comment the line above.
        // Create a new instance of the SignatureHandler.
        //OpenSSLSignatureHandler sigHandler(certfile.ConvertToUtf8().c_str(), "password");
        // Add the SignatureHandler instance to PDFDoc, making sure to keep track of it using the ID returned.
        //SDF::SignatureHandlerId sigHandlerId = doc.AddSignatureHandler(sigHandler);
        
        // Create new signature form field in the PDFDoc.
        Field sigField = doc.FieldCreate(UString("Signature1"), Field::e_signature);

        Page page1 = doc.GetPage(1);
        Widget widgetAnnot = pdftron::PDF::Annots::Widget::Create(doc.GetSDFDoc(), Rect(0, 0, 0, 0), sigField);
        page1.AnnotPushBack(widgetAnnot);
        widgetAnnot.SetPage(page1);
        SDF::Obj widgetObj = widgetAnnot.GetSDFObj();
        widgetObj.PutNumber("F", 132);
        widgetObj.PutName("Type", "Annot");
        
        // Tell PDFNetC to use the SignatureHandler created to sign the new signature form field.
        SDF::Obj sigDict = sigField.UseSignatureHandler(sigHandlerId);
        
        // Add more information to the signature dictionary.
        sigDict.PutName("SubFilter", "adbe.pkcs7.detached");
        sigDict.PutString("Name", "PDFTron");
        sigDict.PutString("Location", "Vancouver, BC");
        sigDict.PutString("Reason", "Document verification.");

        // Appearance can be added to the widget annotation. Please see the "SignPDF()" function for details.

        // Add this sigDict as DocMDP in Perms dictionary from root
        SDF::Obj root = doc.GetRoot();
        SDF::Obj perms = root.PutDict("Perms");
        // add the sigDict as DocMDP (indirect) in Perms
        perms.Put("DocMDP", sigDict);
        
        // add the additional DocMDP transform params
        SDF::Obj refObj = sigDict.PutArray("Reference");
        SDF::Obj transform = refObj.PushBackDict();
        transform.PutName("TransformMethod", "DocMDP");
        transform.PutName("Type", "SigRef");
        SDF::Obj transformParams = transform.PutDict("TransformParams");
        transformParams.PutNumber("P", 1); // Set permissions as necessary.
        transformParams.PutName("Type", "TransformParams");
        transformParams.PutName("V", "1.2");

        // Save the PDFDoc. Once the method below is called, PDFNetC will also sign the document using the information
        // provided.
        doc.Save(outfile, 0, NULL);

        cout << "Finished certifying PDF document.\n";
    }
    catch (Common::Exception& e) {
        cerr << e << "\n";
        result = false;
    }
    catch (exception& e) {
        cerr << e.what() << "\n";
        result = false;
    }
    catch (...) {
        cerr << "Unknown exception.\n";
        result = false;
    }

    return (result);
}

int main(int argc, char** argv)
{
    // Initialize PDFNetC
    PDFNet::Initialize();
    
#if (!USE_STD_SIGNATURE_HANDLER)
    // Initialize OpenSSL library
    CRYPTO_malloc_init();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
#endif // (!USE_STD_SIGNATURE_HANDLER)
 
    bool result = true;

    if (!SignPDF())
        result = false;

    if (!CertifyPDF())
        result = false;
    
    // Clean up code
    PDFNet::Terminate();

#if (!USE_STD_SIGNATURE_HANDLER)
    ERR_free_strings();
    EVP_cleanup();
#endif // (!USE_STD_SIGNATURE_HANDLER)

    if (!result) {
        cout << "\nTests failed.\n";
        return 1;
    }

    cout << "\nAll tests passed.\n";

    return 0;
}
