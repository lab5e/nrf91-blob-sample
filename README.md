# nRF9160 blob upload sample

This is an example of blob uploads with nRF9160.

Register the device in Span as you'd normally do, then build and launch the sample. The blob with auto-generated data is uploaded to Span.

Please note that this code isn't production quality - the HTTP client in Zephyr is probably a better choice if you want to do a proper implementation.

The code in http.c assumes a certain order of calls and might block forever (or fail) if the headers are incorrectly set.
