# Boolean Isolated Searchable Encryption (BISEN)

BISEN is a searchable symmetric encryption scheme, allowing for secure boolean queries over remote datastores by leveraging Intel SGX for trustworthy computation.

A full description of this project was published here: https://ieeexplore.ieee.org/abstract/document/9149632 (alternate link: http://www.di.fc.ul.pt/~blferreira/papers/tsdc20.pdf).

If you are to use this project, please cite: Ferreira, B., Portela, B., Oliveira, T., Borges, G., Domingos, H. J., & Leitao, J. (2020). Boolean Searchable Symmetric Encryption with Filters on Trusted Hardware. IEEE Transactions on Dependable and Secure Computing.

----

The framework exports three utility namespaces (trusted_util, outside_util & tcrypto) that can be used by the external library.

The external library must implement the namespaces defined in the *extern_lib.h* file.

## Namespaces and utility libraries

### Trusted Util

Utility functions (threading, time) inside the enclave.

```namespace trusted_util```

```#include "trusted_util.h"```


### Trusted Crypto

Crypto primitives inside the enclave.

```namespace tcrypto```

```#include "trusted_crypto.h"```


### Outside Util

Wrappers for ocalls, providing an abstraction for common functions, to be used inside the enclave.

```namespace outside_util```

```#include "outside_util.h"```

---

### Untrusted Util

Utility functions (i.e. time) outside the enclave.

```namespace untrusted_util```

```#include "untrusted_util.h"```


### Extern Lib

Functions to be implemented in the external lib.

```namespace extern_lib; namespace extern_lib_ut```

```#include "extern_lib.h"```
