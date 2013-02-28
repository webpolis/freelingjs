#include "init.h"

char * convert(const char *from_charset, const char *to_charset, const char *input) {
    size_t inleft, outleft, converted = 0;
    char *output, *outbuf, *tmp;
    const char *inbuf;
    size_t outlen;
    iconv_t cd;

    if ((cd = iconv_open(to_charset, from_charset)) == (iconv_t) - 1)
        return NULL;

    inleft = strlen(input);
    inbuf = input;

    /* we'll start off allocating an output buffer which is the same size
     * as our input buffer. */
    outlen = inleft;

    /* we allocate 4 bytes more than what we need for nul-termination... */
    if (!(output = malloc(outlen + 4))) {
        iconv_close(cd);
        return NULL;
    }

    do {
        errno = 0;
        outbuf = output + converted;
        outleft = outlen - converted;

        converted = iconv(cd, (char **) &inbuf, &inleft, &outbuf, &outleft);
        if (converted != (size_t) - 1 || errno == EINVAL) {
            /*
             * EINVAL  An  incomplete  multibyte sequence has been encoun­-
             *         tered in the input.
             *
             * We'll just truncate it and ignore it.
             */
            break;
        }

        if (errno != E2BIG) {
            /*
             * EILSEQ An invalid multibyte sequence has been  encountered
             *        in the input.
             *
             * Bad input, we can't really recover from this. 
             */
            iconv_close(cd);
            free(output);
            return NULL;
        }

        /*
         * E2BIG   There is not sufficient room at *outbuf.
         *
         * We just need to grow our outbuffer and try again.
         */

        converted = outbuf - output;
        outlen += inleft * 2 + 8;

        if (!(tmp = realloc(output, outlen + 4))) {
            iconv_close(cd);
            free(output);
            return NULL;
        }

        output = tmp;
        outbuf = output + converted;
    } while (1);

    /* flush the iconv conversion */
    iconv(cd, NULL, NULL, &outbuf, &outleft);
    iconv_close(cd);

    /* Note: not all charsets can be nul-terminated with a single
     * nul byte. UCS2, for example, needs 2 nul bytes and UCS4
     * needs 4. I hope that 4 nul bytes is enough to terminate all
     * multibyte charsets? */

    /* nul-terminate the string */
    memset(outbuf, 0, 4);

    return output;
}

void Freeling::init(Handle<Object> target) {
    HandleScope scope;

    // initialize freeling
    util::init_locale(L"default");

    // initialize module variables
    char out [21];
    int n = sprintf(out, "Freeling for NodeJS - version: %i.%i - author: Nicolas Iglesias <nico@webpolis.com.ar>", 1, 0);
    target->Set(v8::String::NewSymbol("version"), v8::String::New(out, n));

    NODE_SET_METHOD(target, "process", process);
}

Handle<Value> Freeling::process(const Arguments &args) {
    HandleScope scope;

    char* tokens;
    ARG_STRING_TO_CHAR(0, tokens);

    list<word> lw;
    list<sentence> ls;
    wstring path = L"/usr/local/share/freeling/es/";

    tokenizer tk(path + L"tokenizer.dat");
    splitter sp(path + L"splitter.dat");

    // morphological analysis has a lot of options, and for simplicity they are packed up
    // in a maco_options object. First, create the maco_options object with default values.
    maco_options opt(L"es");
    // then, set required options on/off  
    opt.UserMap = false;
    opt.QuantitiesDetection = true; //deactivate ratio/currency/magnitudes detection 
    opt.AffixAnalysis = true;
    opt.MultiwordsDetection = true;
    opt.NumbersDetection = true;
    opt.PunctuationDetection = true;
    opt.DatesDetection = true;
    opt.DictionarySearch = true;
    opt.ProbabilityAssignment = true;
    opt.NERecognition = true;

    // and provide files for morphological submodules. Note that it is not necessary
    // to set opt.QuantitiesFile, since Quantities module was deactivated.
    opt.UserMapFile = L"";
    opt.LocutionsFile = path + L"locucions.dat";
    opt.AffixFile = path + L"afixos.dat";
    opt.ProbabilityFile = path + L"probabilitats.dat";
    opt.DictionaryFile = path + L"dicc.src";
    opt.NPdataFile = path + L"np.dat";
    opt.PunctuationFile = path + L"../common/punct.dat";
    opt.QuantitiesFile = path + L"quantities.dat";

    // create the analyzer with the just build set of maco_options
    maco morfo(opt);
    // create a hmm tagger for spanish (with retokenization ability, and forced 
    // to choose only one tag per word)
    hmm_tagger tagger(path + L"tagger.dat", true, FORCE_TAGGER);

    //tokens = convert("UTF-8","ISO-8859-1",tokens);
    //setlocale(LC_CTYPE, "iso-8859-1");
    lw = tk.tokenize(util::string2wstring(tokens));
    ls = sp.split(lw, false);
    //    cout << "=========================================================" << endl
    //            << tokens << endl
    //            << "=========================================================" << endl;

    cout << "Analyzing " << lw.size() << " token(s) distributed in " << ls.size() << " sentence(s)" << endl;

    // perform and output morphosyntactic analysis and disambiguation
    morfo.analyze(ls);
    tagger.analyze(ls);

    // collect results
    word::const_iterator a;
    sentence::const_iterator w;

    Local<Array> ret = Array::New(0);
    int c = 0;

    for (list<sentence>::iterator is = ls.begin(); is != ls.end(); is++) {
        for (w = is->begin(); w != is->end(); w++) {
            Local<Object> word = Object::New();
            Local<Array> analysis = Array::New(0);

            word->Set(String::NewSymbol("lemma"), String::NewSymbol(util::wstring2string(w->get_lemma()).c_str()));
            word->Set(String::NewSymbol("form"), String::NewSymbol(util::wstring2string(w->get_form()).c_str()));

            int cc = 0;
            // for each possible analysis in word, output lemma, tag and probability
            for (a = w->analysis_begin(); a != w->analysis_end(); ++a) {
                Local<Object> aw = Object::New();
                aw->Set(String::NewSymbol("lemma"), String::NewSymbol(util::wstring2string(a->get_lemma()).c_str()));
                aw->Set(String::NewSymbol("prob"), Number::New(a->get_prob()));
                aw->Set(String::NewSymbol("pos"), String::NewSymbol(util::wstring2string(a->get_tag()).c_str()));
                analysis->Set(Integer::New(cc++), aw);
            }
            word->Set(String::NewSymbol("analysis"), analysis);
            ret->Set(Integer::New(c++), word);
        }
    }

    // clear temporary lists;
    lw.clear();
    ls.clear();

    // prepare callback
    REQ_FUN_ARG(1, cb);
    Local<Value> argv[2];
    argv[0] = Local<Value>::New(ret);
    argv[1] = Local<Value>::New(args[0]->ToString());

    TryCatch try_catch;

    cb->Call(Context::GetCurrent()->Global(), 2, argv);

    return scope.Close(Null());
}

NODE_MODULE(freeling, Freeling::init)
