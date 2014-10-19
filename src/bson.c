#include <Rinternals.h>
#include <bson.h>
#include <mongoc.h>
#include <utils.h>

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter);
SEXP ConvertValue(bson_iter_t* iter);

SEXP R_json_to_bson(SEXP json){
  bson_t *b;
  bson_error_t err;

  b = bson_new_from_json ((uint8_t*)translateCharUTF8(asChar(json)), -1, &err);
  if(!b)
    error(err.message);

  return bson2r(b);
}

SEXP R_raw_to_bson(SEXP buf){
  bson_t *b;
  bson_error_t err;
  int len = length(buf);
  uint8_t data[len];

  for (int i = 0; i < len; i++) {
    data[i] = RAW(buf)[i];
  }

  b = bson_new_from_data(data, len);
  if(!b)
    error(err.message);

  return bson2r(b);
}

SEXP R_bson_to_json(SEXP ptr){
  return mkStringUTF8(bson_as_json (r2bson(ptr), NULL));
}

SEXP R_bson_to_raw(SEXP ptr){
  bson_t *b = r2bson(ptr);
  const uint8_t *buf = bson_get_data(b);
  int len = b->len;

  SEXP out = PROTECT(allocVector(RAWSXP, len));
  for (int i = 0; i < len; i++) {
    RAW(out)[i] = buf[i];
  }
  UNPROTECT(1);
  return out;
}

SEXP R_bson_to_list(SEXP ptr) {
  bson_t *b = r2bson(ptr);
  bson_iter_t iter1;
  bson_iter_t iter2;
  bson_iter_init(&iter1, b);
  bson_iter_init(&iter2, b);
  return ConvertObject(&iter1, &iter2);
}

SEXP ConvertValue(bson_iter_t* iter){
  if(BSON_ITER_HOLDS_INT32(iter)){
    return ScalarInteger(bson_iter_int32(iter));
  } else if(BSON_ITER_HOLDS_NULL(iter)){
    return R_NilValue;
  } else if(BSON_ITER_HOLDS_BOOL(iter)){
    return ScalarLogical(bson_iter_bool(iter));
  } else if(BSON_ITER_HOLDS_DOUBLE(iter)){
    return ScalarReal(bson_iter_double(iter));
  } else if(BSON_ITER_HOLDS_INT64(iter)){
    return ScalarReal((double) bson_iter_int64(iter));
  } else if(BSON_ITER_HOLDS_UTF8(iter)){
    return mkStringUTF8(bson_iter_utf8(iter, NULL));
  } else if(BSON_ITER_HOLDS_ARRAY(iter)){
    bson_iter_t child1;
    bson_iter_t child2;
    bson_iter_recurse (iter, &child1);
    bson_iter_recurse (iter, &child2);
    return ConvertArray(&child1, &child2);
  } else if(BSON_ITER_HOLDS_DOCUMENT(iter)){
    bson_iter_t child1;
    bson_iter_t child2;
    bson_iter_recurse (iter, &child1);
    bson_iter_recurse (iter, &child2);
    return ConvertObject(&child1, &child2);
  } else {
    error("Unimplemented BSON type %d\n", bson_iter_type(iter));
  }
}

SEXP ConvertArray(bson_iter_t* iter, bson_iter_t* counter){
  SEXP ret;
  int count = 0;
  while(bson_iter_next(counter)){
    count++;
  }
  PROTECT(ret = allocVector(VECSXP, count));
  for (int i = 0; bson_iter_next(iter); i++) {
    SET_VECTOR_ELT(ret, i, ConvertValue(iter));
  }
  UNPROTECT(1);
  return ret;
}

SEXP ConvertObject(bson_iter_t* iter, bson_iter_t* counter){
  SEXP names;
  SEXP ret;
  int count = 0;
  while(bson_iter_next(counter)){
    count++;
  }
  PROTECT(ret = allocVector(VECSXP, count));
  PROTECT(names = allocVector(STRSXP, count));
  for (int i = 0; bson_iter_next(iter); i++) {
    SET_STRING_ELT(names, i, mkChar(bson_iter_key(iter)));
    SET_VECTOR_ELT(ret, i, ConvertValue(iter));
  }
  setAttrib(ret, R_NamesSymbol, names);
  UNPROTECT(2);
  return ret;
}
