
#include <string> 

class JSMNobject {
	private:
	 const char * json_string ;
	 jsmntok_t * pt ;
	 uint8_t iterator;
	 jsmntok_t * piterator ;
	 jsmntype_t type ;
	 
	static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
		if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
		}
		return -1;
	};
	 
	 
	 static int count_tokens(jsmntok_t * pt) {
		if (pt->type == JSMN_STRING || pt->type == JSMN_PRIMITIVE) { return 1 ;}
		if (pt->type == JSMN_OBJECT) {
			int count = 1 ;
			int elements = pt->size ;

			pt += 1 ;
			for (int i = 0 ; i < elements ; i++){
				char id;
//				printf("[O<%d,%d>,<%c%c%c%c...%c>=",pt->start, pt->end, json_string[pt->start], json_string[pt->start+1], json_string[pt->start+2],json_string[pt->start+3],json_string[pt->end - 1]);
				int this_element = 1 + count_tokens(pt + 1) ;
				count += this_element ;
				pt += this_element ;
			};
			return count ;
		}
		if (pt->type == JSMN_ARRAY) {
			int count = 1 ;
			int elements = pt->size;
			pt = pt + 1 ;
			for (int i = 0 ; i < elements ; i++) {
				int this_element = count_tokens(pt);
				count += this_element ;
				pt += this_element ;
			};
			return count;
		};
		return -1 ;
	};
		
	 
	public:
	JSMNobject(const char * str, jsmntok_t * token_start) {
		if (token_start == NULL || str == NULL) {
			type = JSMN_UNDEFINED ;
			pt = NULL ;
			piterator = NULL ;
			return;
		};
		pt = token_start ;
		json_string = (char * )str ;
		iterator = 0 ;
		piterator = pt + 1 ;
		type = pt->type ;
//		printf("DEBUG creating object <%d,%d>, iterator = %d, piterator =<%d,%d>, type = %d\n",pt->start, pt->end, iterator, piterator->start, piterator->end, pt->type); 
	};
	

	
	jsmntype_t isOf() {
		return type;
	}
	
	bool isUndef() {
		return type == JSMN_UNDEFINED;
	}
	
	int asInt() {
			if (type != JSMN_PRIMITIVE) return 0 ;
			return atoi(json_string + pt->start) ;
	}
	
	int asString(char * str, int maxlength) {
		if (type == JSMN_UNDEFINED) { str[0] = '\0' ; return 0;}
		int no_chars = (maxlength <= (pt->end - pt->start))?maxlength:(pt->end - pt->start) ;
		memcpy(str,json_string + pt->start,no_chars);
		str[no_chars] = 0 ;
		return no_chars ;
	}
	
	bool asBool() {
		return ( (pt != NULL) && (json_string + pt->start)[0] == 't') ;
	};
	
	bool asNULL() {
		return ( (pt != NULL) && (json_string + pt->start)[0] == 'n') ;
	};
	
	jsmntok_t * asToken() {
		return pt;
	}
	
	int sizeIter() {
		if (type != JSMN_ARRAY && type != JSMN_OBJECT) return -1 ;
		return pt->size ;
	}
	
	void iterate() {
		if (type != JSMN_ARRAY && type != JSMN_OBJECT) return ;
		iterator = (iterator + 1) % pt->size ;
		if (iterator == 0 ) {
			piterator = pt + 1;
			return;
		};
		if (type == JSMN_OBJECT) {
//			printf("DEBUG: ITERATE OVER OBJECT (%d), <%d,%d>\n", iterator, piterator->start, piterator->end);
			piterator = piterator + 1 + count_tokens(piterator + 1) ;
//			printf("\nDEBUG: ITERATE OVER OBJECT NEXT = <%d,%d>\n",piterator->start, piterator->end);			
		} else {
//			printf("DEBUG: ITERATE OVER ARRAY (%d), <%d,%d>\n", iterator, piterator->start, piterator->end);
			int count_t = count_tokens(piterator) ;
			piterator = piterator + count_t ;
//			printf("\n\nDEBUG: ITERATE OVER ARRAY NEXT = <%d,%d>\n",piterator->start, piterator->end);
		};
		return;
	}
	
	void iterKey(char * str, int maxlength) {
		if (type != JSMN_OBJECT) { str[0] = '\0' ; return;}
		jsmntok_t * key = piterator;
		int no_chars = (maxlength <= (key->end - key->start))?maxlength:(key->end - key->start) ;
		memcpy(str,json_string + key->start,no_chars);
		str[no_chars] = 0 ;
	}

    JSMNobject iterElem() {
		JSMNobject undef_object(NULL,NULL) ;
		if (type == JSMN_OBJECT) {
			JSMNobject retour(json_string,piterator + 1) ;
			return retour ;
		} else if (type == JSMN_ARRAY) {
			JSMNobject retour(json_string,piterator) ;
			return retour;
		}
		return undef_object ;
	};
	
	JSMNobject select (const char * str) {
		JSMNobject undef_object(NULL,NULL) ;
		if (type != JSMN_OBJECT) return undef_object ;
		for (int i = 0 ; i < pt->size; i++ ) {
			if (jsoneq(json_string,piterator,str) == 0) {
				JSMNobject retour(json_string, piterator + 1) ;
				return retour ;
			};
			iterate() ;
		};
		return undef_object ;
	};
	
	JSMNobject element(int el) {
		JSMNobject undef_object(NULL,NULL) ;
		if (type != JSMN_ARRAY) return undef_object ;
		for (int i = 0 ; i < pt->size; i++ ) {
			if (iterator == el) {
				JSMNobject retour(json_string, piterator) ;
				return retour ;
			};
			iterate() ;
		};
		return undef_object ;
	};
};