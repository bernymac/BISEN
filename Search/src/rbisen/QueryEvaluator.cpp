#include "QueryEvaluator.h"

#include "outside_util.h"

// receives a set of docs and returns its negation in the
// full docs set; equivalent to the set differentiation:
// {all-docs} \ {negate}
vec_int get_not_docs(int nDocs, vec_int negate, unsigned char* count){
    //unsigned char *count = (unsigned char*) malloc(sizeof(unsigned char) * nDocs);

    unsigned int nops = 0;

    memset(count, 0, sizeof(unsigned char) * nDocs);

    //vi_print(negate);

    // increase the count for elems in the original vector
    for(unsigned i = 0; i < vi_size(&negate); i++) {
        //printf("aa %p\n", negate.array);
        count[negate.array[i]] =1;
        nops++;
    }

    // all elements that have count == 0 are the negation of the set
    //vector<int> result(nDocs - negate.size());
    vec_int result;
    vi_init(&result, nDocs - nops);

    //int res_count = 0;
    for(int i = 0; i < nDocs; i++) {
        if(count[i] == 0) {
           vi_push_back(&result, i);
        }
    }

    //free(count);
    return result;
}

// evaluates a query in reverse polish notation, returning
// the resulting set of docs
vec_int evaluate(vec_token rpn_expr, int nDocs, unsigned char* count) {
    vec_token eval_stack;
    vt_init(&eval_stack, MAX_QUERY_TOKENS);

    iee_token tkn;
    for(unsigned i = 0; i < vt_size(&rpn_expr); i++) {
        tkn = rpn_expr.array[i];

        if(tkn.type == '&') {
            if (vt_size(&eval_stack) < 2) {
                outside_util::printf("Insufficient operands for AND!\n");
                outside_util::exit(-1);
            }

            // get both operands for AND
            vec_int and1 = vt_peek_back(eval_stack).docs;
            vt_pop_back(&eval_stack);

            vec_int and2 = vt_peek_back(eval_stack).docs;
            vt_pop_back(&eval_stack);

            // intersection of the two sets of documents
            vec_int set_inter = vi_vec_intersection(and1, and2, count, nDocs);

            /*ocall_strprint("intersection ");
            for(int i = 0; i < size(set_inter); i++)
                ocall_printf("%i ", set_inter.array[i]);
            ocall_strprint("\n");*/

            iee_token res;
            res.type = 'r';
            res.docs = set_inter;

            vt_push_back(&eval_stack, &res);

            // free memory
            vi_destroy(&and1);
            vi_destroy(&and2);
        } else if(tkn.type == '|') {
            if (vt_size(&eval_stack) < 2) {
                outside_util::printf("Insufficient operands for OR!\n");
                outside_util::exit(-1);
            }

            // get both operands for OR
            vec_int or1 = vt_peek_back(eval_stack).docs;
            vt_pop_back(&eval_stack);

            vec_int or2 = vt_peek_back(eval_stack).docs;
            vt_pop_back(&eval_stack);

            // union of the two sets of documents
            vec_int set_un = vi_vec_union(or1, or2, count, nDocs);
            //set_union(or1.begin(), or1.end(), or2.begin(), or2.end(), back_inserter(set_un));

            /*ocall_strprint("union ");
            for(int i = 0; i < size(set_un); i++)
                ocall_printf("%i ", set_un.array[i]);
            ocall_strprint("\n");*/

            iee_token res;
            res.type = 'r';
            res.docs = set_un;

            vt_push_back(&eval_stack, &res);

            // free memory
            vi_destroy(&or1);
            vi_destroy(&or2);
        } else if(tkn.type == '!') {
            if (vt_size(&eval_stack) < 1) {
                outside_util::printf("Insufficient operands for NOT!\n");
                outside_util::exit(-1);
            }

            vec_int negate = vt_peek_back(eval_stack).docs;
            // printf("negation");
            //vi_print(negate);
            vt_pop_back(&eval_stack);

            // difference between all docs and the docs we don't want
            vec_int set_diff = get_not_docs(nDocs, negate, count);

            /*ocall_strprint("not ");
            for(int i = 0; i < size(set_diff); i++)
                ocall_printf("%i ", set_diff.array[i]);
            ocall_strprint("\n");*/

            iee_token res;
            res.type = 'r';
            res.docs = set_diff;

            vt_push_back(&eval_stack, &res);

            // free memory
            vi_destroy(&negate);
        } else {
            //vi_print(tkn.docs);
            vt_push_back(&eval_stack, &tkn);
        }
    }

    if (vt_size(&eval_stack) != 1) {
        outside_util::printf("Wrong number of operands left\n");
        //printf("Wrong number of operands left: %u\n", vt_size(eval_stack));
        //ocall_printf("%02x\n", vt_peek_back(eval_stack).type);
        outside_util::exit(-1);
    }

    //return vt_peek_back(eval_stack).docs;
    vec_int ret = vt_peek_back(eval_stack).docs;
    vt_destroy(&eval_stack);
    return ret;
}
