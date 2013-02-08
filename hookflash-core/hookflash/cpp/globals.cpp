/*
 * Beach Path Inc. - hookflash Client API
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Contributor(s):
 *
 * Bojan Lazic <bojan@yakolako.com>
 *
 */

#include <hookflash/internal/globals.h>

namespace hookflash {
  namespace sip {
    namespace internal {
      bool GlobalsPJSIP::callAutoAnswer = false;

      GlobalsPJSIP &getGlobals() {
        GlobalsPJSIP ret;
        return ret;
      }
    }
  }
}