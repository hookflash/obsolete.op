/* 

Copyright (c) 2012, SMB Phone Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

 */

/*
 *  This webpage is	loaded as a	hidden inner iframe	by the browser and receives
 *  information	via	the outer JavaScript via a message posted to the identity 
 *  provider's inner JavaScript hidden iframe window page.The URL for this page 
 *  to render in a browser is returned from the "Identity Login Start" page. 
 *  Normally the inner page receives the private posted information and immediately 
 *  redirects the outer page to the identity provider's login page allowing the user
 *  to enter their identity	credentials. This the outer page is typically rendered 
 *  inside a browser window and contains sufficient display size to allow an 
 *  identity provider to enter their credential	information	(unless	relogin	is used	
 *  in which case there will be no rendered page for entering credential information).
 */

//STATES
var isIframeReady = false;

// VARIABLES
var notifyBundle;

// inner frame
var innerFrame;

// Run init method on load
initOuter();

// Init function
// called on load page
function initOuter() {
    // load data from storage
    if (typeof (Storage) !== "undefined") {
        notifyBundle = localStorage.notifyBundle;
        var message = {
            message : "outer-loaded"
        };
        notifyClient();
    } else {
        // No localStorage support.
        var message = {
            error : "Fatal error - no localStorage support"
        };
        notifyClient(message);
    }
}

// Init inner frame metod.
// called by client application
// after outer page is loaded.
//
// @param notifyBundle
// @param identityLoginURL - location of inner frame
function initInnerFrame(notifyBundle, identityLoginURL) {
    // load inner frame
    var child = document.createElement('iframe');
    child.src = "https://" + identityLoginURL;

    var body = document.getElementsByTagName('body')[0];
    body.appendChild(child);
}

// Global cross-domain message receiver
window.onmessage = function(message) {
    if (message.data === 'ready') {
        // when iframe is loaded
        innerFrame = document.getElementsByTagName('iframe')[0];
        // notify client
        // notifyInner();
    } else {
        // parse json
        var data = JSON.parse(message.data);
        // handle data

    }
}

// Notifies client application when inner and outer frames are ready
// client should call initOuterFrame() method as response.
// @param message
function notifyClient(message) {
    var iframe = document.createElement("IFRAME");
    iframe.setAttribute("src", "hookflash-js-frame:notifyClient:" + message);
    document.documentElement.appendChild(iframe);
    iframe.parentNode.removeChild(iframe);
    iframe = null;
}