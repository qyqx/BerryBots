/*
  Copyright (C) 2013 - Voidious

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <stdio.h>
#include <string.h>
#include "zipper.h"

ZipperException::ZipperException(const char *details) {
  message_ = new char[strlen(details) + 41];
  sprintf(message_, "Zip processing failure: %s", details);
}

const char* ZipperException::what() const throw() {
  return message_;
}

ZipperException::~ZipperException() throw() {
  delete message_;
}
