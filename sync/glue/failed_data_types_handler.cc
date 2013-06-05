// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/data_type_manager.h"
#include "chrome/browser/sync/glue/failed_data_types_handler.h"

using browser_sync::DataTypeManager;

namespace browser_sync {

namespace {

syncer::ModelTypeSet GetTypesFromErrorMap(
    const FailedDataTypesHandler::TypeErrorMap& errors) {
  syncer::ModelTypeSet result;
  for (FailedDataTypesHandler::TypeErrorMap::const_iterator it = errors.begin();
       it != errors.end(); ++it) {
    DCHECK(!result.Has(it->first));
    result.Put(it->first);
  }
  return result;
}

}  // namespace

FailedDataTypesHandler::FailedDataTypesHandler() {
}

FailedDataTypesHandler::~FailedDataTypesHandler() {
}

syncer::ModelTypeSet FailedDataTypesHandler::GetFailedTypes() const {
  syncer::ModelTypeSet result = GetTypesFromErrorMap(startup_errors_);
  result.PutAll(GetTypesFromErrorMap(runtime_errors_));
  return result;
}

bool FailedDataTypesHandler::UpdateFailedDataTypes(
    const TypeErrorMap& errors,
    FailureType failure_type) {
  if (failure_type == RUNTIME) {
    runtime_errors_.insert(errors.begin(), errors.end());
  } else if (failure_type == STARTUP) {
    startup_errors_.insert(errors.begin(), errors.end());
  } else {
    NOTREACHED();
  }

  return !errors.empty();
}

void FailedDataTypesHandler::Reset() {
  startup_errors_.clear();
  runtime_errors_.clear();
}

FailedDataTypesHandler::TypeErrorMap FailedDataTypesHandler::GetAllErrors()
    const {
  TypeErrorMap result;

  if (AnyFailedDatatype()) {
    result = startup_errors_;
    result.insert(runtime_errors_.begin(), runtime_errors_.end());
  }
  return result;
}

bool FailedDataTypesHandler::AnyFailedDatatype() const {
  return (!startup_errors_.empty() || !runtime_errors_.empty());
}

}  // namespace browser_sync
