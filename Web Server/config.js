  $(function() {
    $(document).on('documentLoaded', async function onDocumentLoaded() {
      await PDFNet.initialize();
      var doc = readerControl.docViewer.getDocument();
      var pdfDoc = await doc.getPDFDoc();

      // Ensure that we have our first page.
      await pdfDoc.requirePage(1);

      // Refresh the cache with the newly updated document
      readerControl.docViewer.refreshAll();
      // Update viewer with new document
      readerControl.docViewer.updateView();
    });
  });
