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

/**
 *      This webpage is loaded as a hidden inner iframe by the browser and receives
 *  information via the outer JavaScript via a message posted to the identity 
 *  provider's inner JavaScript hidden iframe window page.The URL for this page 
 *  to render in a browser is returned from the "Identity Login Start" page. 
 *  Normally the inner page receives the private posted information and immediately 
 *  redirects the inner page to the identity provider's login page allowing the user
 *  to enter their identity credentials. This the outer page is typically rendered 
 *  inside a browser window and contains sufficient display size to allow an 
 *  identity provider to enter their credential information (unless relogin is used 
 *  in which case there will be no rendered page for entering credential information).
 *  
 *  Methods:
 *      init                                - loads inner frame (called on load)
 *      initInnerFrame                      - called by client app
 *      sendBundleToJS                      - called by client app
 *      notifyClient                        - sends message to client application
 */

// CONSTANTS
var innerFrameId = "innerFrameId";       // inner iframe id
var innerFrameParentId = "innerFrame";   // inner iframe id

// Variables
var inner;
var innerFrameDomain;
var innerFrameURL;                       // inner iframe url
var initData;

/** 
 * Init method.
 * onload page
 */ 
function init() {
    var url = window.location.href;
    initData = {outerURL: url};
}

/**
 * Initialize inner frame.
 * called by client application
 * after outer page is loaded.
 *
 * @param identityLoginURL - url of the inner frame
 */
function initInnerFrame(identityLoginURL) {
    logMe('initInnerFrame' + identityLoginURL);
    innerFrameURL = identityLoginURL;
    localStorage.innerFrameURL = innerFrameURL;
    // load inner frame
    var innerFrame = document.createElement('iframe');
    innerFrame.src = innerFrameURL;
    innerFrame.id = innerFrameId;
    var parentDiv = document.getElementById(innerFrameParentId);
    parentDiv.appendChild(innerFrame);
    innerFrame.style.width = "100%";
    innerFrame.style.height = "100%";
}

/**
 * Global cross-domain message receiver.
 *
 * @param message
 */
window.onmessage = function(message) {
    logMe('window.onmessage - start' + message.data);
    handleOnMessage(message);
}

/**
 * Handle window.onmessage.
 *
 * @param message
 */
function handleOnMessage(message) {
    try {
        var dataJSON = JSON.parse(message.data);
        if (dataJSON.notify) {
            //localStorage.outerFrameURL = dataJSON.notify.browser.outerFrameURL;
            notifyClient(JSON.stringify(dataJSON));
        }else if (dataJSON.request) {
            if (dataJSON.request.$method === 'namespace-grant-window'){
                notifyClient(JSON.stringify(dataJSON));
            }else if (dataJSON.request.$method === 'identity-access-lockbox-update'){
                notifyClient(JSON.stringify(dataJSON));
            }
        }else {
            notifyClient(JSON.stringify(dataJSON));
        }
    } catch (e) {
        // handle exception
        var errorMessage = {
            "error" : "error parsing json"
        };
        notifyClient(JSON.stringify(errorMessage));
    }
}

/**
 * Client to javaScript message receiver.
 *
 * @param bundle - notify or result
 */
function sendBundleToJS(bundle){
    logMe('sendBundleToJS -' + bundle);
    try {
        var dataJSON = JSON.parse(bundle);
        inner = document.getElementById(innerFrameId).contentWindow;
        var innerFrameDomainData = innerFrameURL.split("/");
        innerFrameDomain = innerFrameDomainData[2];
        //TODO
        if (location.protocol === 'https:'){
		      locationProtocol = "https:";
	      } else {
		      locationProtocol = "http:";
	      }
        inner.postMessage(bundle, locationProtocol + innerFrameDomain);
    } catch(e){
        logMe('sendBundleToJS - error');
    }
}

/**
 * Notifies client application.
 *
 * @param message
 */
function notifyClient(message) {
    var iframe = document.createElement("IFRAME");
    var locationProtocol;
	  if (location.protocol === 'https:'){
		  locationProtocol = "https:";
	  } else {
		  locationProtocol = "http:";
	  }
    iframe.setAttribute("src", locationProtocol + "//datapass.hookflash.me/?method=notifyClient;data=" + message);
    document.documentElement.appendChild(iframe);
    iframe.parentNode.removeChild(iframe);
    iframe = null;
    console.log(locationProtocol + "//datapass.hookflash.me/?method=notifyClient  data=" + message);
}

// logger
function logMe(msg){
    console.log(msg);
}
