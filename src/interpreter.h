#pragma once

#include "types.h"
#include "variant.h"
#include "binary.h"
#include "paramstack.h"
#include "array.h"
#include "container.h"

struct fake;

#define GET_CMD(fb, pos) fb.m_buff[pos]

#define GET_CONST(v, fb, pos) \
    assert(pos >= 0 && pos < (int)fb.m_const_list_num);\
    v = &fb.m_const_list[pos];

#define GET_CONTAINER(v, s, fb, pos) v = get_container_variant(s, fb, pos)

#define GET_STACK(v, s, pos) \
	assert(pos >= 0 && pos < (int)ARRAY_MAX_SIZE((s).m_stack_variant_list));\
    v = &ARRAY_GET((s).m_stack_variant_list, pos);

#define SET_STACK(v, s, pos) \
	assert(pos >= 0 && pos < (int)ARRAY_MAX_SIZE((s).m_stack_variant_list));\
    ARRAY_GET((s).m_stack_variant_list, pos) = *v;
    
#define GET_VARIANT(s, fb, v, pos) \
    GET_VARIANT_BY_CMD(s, fb, v, GET_CMD(fb, pos))
    
#define GET_VARIANT_BY_CMD(s, fb, v, cmd) \
    command v##_cmd = cmd;\
    assert (COMMAND_TYPE(v##_cmd) == COMMAND_ADDR);\
    int v##_addrtype = ADDR_TYPE(COMMAND_CODE(v##_cmd));\
    int v##_addrpos = ADDR_POS(COMMAND_CODE(v##_cmd));\
    assert (v##_addrtype == ADDR_STACK || v##_addrtype == ADDR_CONST || v##_addrtype == ADDR_CONTAINER);\
    if (v##_addrtype == ADDR_STACK)\
    {\
        GET_STACK(v, s, (v##_addrpos));\
    }\
    else if (v##_addrtype == ADDR_CONST)\
    {\
		GET_CONST(v, fb, (v##_addrpos)); \
    }\
    else if (v##_addrtype == ADDR_CONTAINER)\
    {\
		GET_CONTAINER(v, s, fb, (v##_addrpos)); \
    }\
    else\
    {\
		v = 0;\
        assert(0);\
        FKERR("next_assign assignaddrtype cannot be %d %d", v##_addrtype, v##_addrpos);\
        err = true;\
        break;\
    }

#define LOG_VARIANT(s, fb, pos, prefix) \
    FKLOG(prefix " variant %d %d", \
        ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, pos))),\
        ADDR_POS(COMMAND_CODE(GET_CMD(fb, pos))));

#define MATH_OPER(s, fb, oper) \
	const variant * left = 0;\
	LOG_VARIANT(s, fb, (s).m_pos, "left");\
    GET_VARIANT(s, fb, left, (s).m_pos);\
    (s).m_pos++;\
    \
	const variant * right = 0;\
	LOG_VARIANT(s, fb, (s).m_pos, "right");\
    GET_VARIANT(s, fb, right, (s).m_pos);\
    (s).m_pos++;\
    \
	variant * dest;\
    assert (ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, (s).m_pos))) == ADDR_STACK);\
    LOG_VARIANT(s, fb, (s).m_pos, "dest");\
	GET_VARIANT(s, fb, dest, (s).m_pos);\
    (s).m_pos++;\
    \
	FKLOG("math left %s right %s", (vartostring(left)).c_str(), (vartostring(right)).c_str());\
    \
    V_##oper(dest, left, right);\
    \
    FKLOG("math %s %s", OpCodeStr(code), (vartostring(dest)).c_str());
 
#define MATH_ASSIGN_OPER(s, fb, oper) \
	variant * var = 0;\
    assert (ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, (s).m_pos))) == ADDR_STACK || ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, (s).m_pos))) == ADDR_CONTAINER);\
	LOG_VARIANT(s, fb, (s).m_pos, "var");\
    GET_VARIANT(s, fb, var, (s).m_pos);\
    (s).m_pos++;\
    \
	const variant * value = 0;\
	LOG_VARIANT(s, fb, (s).m_pos, "value");\
    GET_VARIANT(s, fb, value, (s).m_pos);\
    (s).m_pos++;\
    \
	FKLOG("math var %s value %s", (vartostring(var)).c_str(), (vartostring(value)).c_str());\
    \
    V_##oper(var, var, value);\
    \
    FKLOG("math %s %s", OpCodeStr(code), (vartostring(var)).c_str());

struct stack
{
    fake * m_fk;
    // ����������
    const func_binary * m_fb;
    // ��ǰִ��λ��
    int m_pos;
    // ��ǰջ�ϵı���
	array<variant> m_stack_variant_list;
	// ���õĺ������ر�ջ�ĸ�����λ��
	int m_retnum;
	int m_retvpos[MAX_FAKE_RETURN_NUM];
	// ���ÿ�ʼʱ��
	uint32_t m_calltime;
};

#define STACK_DELETE(s) ARRAY_DELETE((s).m_stack_variant_list)
#define STACK_INI(s, fk, fb) (s).m_fk = fk;\
    (s).m_fb = fb;\
    ARRAY_INI((s).m_stack_variant_list, fk);\
    (s).m_pos = 0

struct processor;
struct interpreter
{
public:
    force_inline bool isend() const
    {
        return m_isend;
    }
    
    void call(binary * bin, const variant & func, paramstack * ps);

    variant * get_container_variant(stack & s, const func_binary & fb, int conpos)
    {
        variant * v = 0;
        assert(conpos >= 0 && conpos < (int)fb.m_container_addr_list_num);
        const container_addr & ca = fb.m_container_addr_list[conpos];
        bool err;
        variant * conv = 0;
        do {GET_VARIANT_BY_CMD(s, fb, conv, ca.con);}while(0);
        const variant * keyv = 0;
        do {GET_VARIANT_BY_CMD(s, fb, keyv, ca.key);}while(0);

        assert(conv->type == variant::ARRAY || conv->type == variant::MAP);
        if (conv->type == variant::ARRAY)
        {
            v = con_array_get(m_fk, conv->data.va, keyv);
        }
        else if (conv->type == variant::MAP)
        {
            v = con_map_get(m_fk, conv->data.vm, keyv);
        }
        assert(v);
        return v;
    }

    force_inline const variant & getret() const
    {
        return m_ret[0];
    }

    force_inline const char * get_running_func_name() const
    {
		return FUNC_BINARY_NAME(*(m_cur_stack->m_fb));
    }

    force_inline int run(int cmdnum)
    {
        int num = 0;
        for (int i = 0; i < cmdnum; i++)
        {
            bool err = false;

            // next
            const func_binary & fb = *m_cur_stack->m_fb;
            int pos = m_cur_stack->m_pos;
            
            // ��ǰ��������
			if (pos >= (int)FUNC_BINARY_CMDSIZE(fb))
            {
				FKLOG("pop stack %s", FUNC_BINARY_NAME(fb));
                // ��¼profile
                endfuncprofile();
                // ��ջ
        		ARRAY_POP_BACK(m_stack_list);
                // ���ж���
            	if (ARRAY_EMPTY(m_stack_list))
                {
                    FKLOG("stack empty end");
                    m_isend = true;
                    break;
                }
            	// ������ֵ
                else
				{
					m_cur_stack = &ARRAY_BACK(m_stack_list);
					const func_binary & fb = *m_cur_stack->m_fb;
					for (int i = 0; i < m_cur_stack->m_retnum; i++)
					{
						variant * ret;
						GET_VARIANT(*m_cur_stack, fb, ret, m_cur_stack->m_retvpos[i]);
						*ret = m_ret[i];
					}
                }
                continue;
            }

            command cmd = GET_CMD(fb, pos);
            int type = COMMAND_TYPE(cmd);
            int code = COMMAND_CODE(cmd);

            USE(type);
            FKLOG("next %d %d %s", type, code, OpCodeStr(code));
                
            assert (type == COMMAND_OPCODE);

            m_cur_stack->m_pos++;

            // ִ�ж�Ӧ�����һ��switchЧ�ʸ��ߣ�cpu�л���
            switch (code)
            {
            case OPCODE_ASSIGN:
                {
                    // ��ֵdest������Ϊջ�ϻ�������
                    assert (ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos))) == ADDR_STACK || 
                        ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos))) == ADDR_CONTAINER);

                    variant * varv = 0;
                    LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "var");
                    GET_VARIANT(*m_cur_stack, fb, varv, m_cur_stack->m_pos);
                    m_cur_stack->m_pos++;
                    
                    // ��ֵ��Դ
                    const variant * valuev = 0;
                    LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "value");
                    GET_VARIANT(*m_cur_stack, fb, valuev, m_cur_stack->m_pos);
                    m_cur_stack->m_pos++;

                    // ��ֵ
                    *varv = *valuev;

                	FKLOG("assign %s to %s", (vartostring(valuev)).c_str(), (vartostring(varv)).c_str());
                }
                break;
            case OPCODE_PLUS:
                {
        		    MATH_OPER(*m_cur_stack, fb, PLUS);
        		}
                break;
            case OPCODE_MINUS:
                {
        		    MATH_OPER(*m_cur_stack, fb, MINUS);
        		}
                break;
            case OPCODE_MULTIPLY:
        		{
        		    MATH_OPER(*m_cur_stack, fb, MULTIPLY);
        		}
                break;
            case OPCODE_DIVIDE:
        		{
        		    MATH_OPER(*m_cur_stack, fb, DIVIDE);
        		}
                break;
            case OPCODE_DIVIDE_MOD:
        		{
        		    MATH_OPER(*m_cur_stack, fb, DIVIDE_MOD);
        		}
                break;
            case OPCODE_AND:
        		{
        		    MATH_OPER(*m_cur_stack, fb, AND);
        		}
                break;
            case OPCODE_OR:
        		{
        		    MATH_OPER(*m_cur_stack, fb, OR);
        		}
                break;
            case OPCODE_LESS:
        		{
        		    MATH_OPER(*m_cur_stack, fb, LESS);
        		}
                break;
        	case OPCODE_MORE:
        		{
        		    MATH_OPER(*m_cur_stack, fb, MORE);
        		}
                break;
        	case OPCODE_EQUAL:
        		{
        		    MATH_OPER(*m_cur_stack, fb, EQUAL);
        		}
                break;
        	case OPCODE_MOREEQUAL:
        		{
        		    MATH_OPER(*m_cur_stack, fb, MOREEQUAL);
        		}
                break;
        	case OPCODE_LESSEQUAL:
        		{
        		    MATH_OPER(*m_cur_stack, fb, LESSEQUAL);
        		}
                break;
        	case OPCODE_NOTEQUAL:
        		{
        		    MATH_OPER(*m_cur_stack, fb, NOTEQUAL);
        		}
                break;
        	case OPCODE_RETURN:
        	    {
					int returnnum = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
					if (!returnnum)
                	{
                		FKLOG("return empty");
		                m_cur_stack->m_pos = fb.m_size;
                		break;
                	}
					m_cur_stack->m_pos++;

                	// ����ret
					for (int i = 0; i < returnnum; i++)
					{
						const variant * ret = 0;
						LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "ret");
						GET_VARIANT(*m_cur_stack, fb, ret, m_cur_stack->m_pos);
						m_cur_stack->m_pos++;

						m_ret[i] = *ret;

						FKLOG("return %s", (vartostring(&m_ret[i])).c_str());
					}
        	    }
        		break;
        	case OPCODE_JNE:
        		{
                	const variant * cmp = 0;
                	LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "cmp");
                	GET_VARIANT(*m_cur_stack, fb, cmp, m_cur_stack->m_pos);
                	m_cur_stack->m_pos++;

                    int pos = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
                	m_cur_stack->m_pos++;
                	
                    if (!(V_BOOL(cmp)))
                    {
                	    FKLOG("jne %d", pos);
                        
                        m_cur_stack->m_pos = pos;
                    }
                    else
                    {
                	    FKLOG("not jne %d", pos);
                    }
        		}
        		break;
        	case OPCODE_JMP:
        		{
                    int pos = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
                	m_cur_stack->m_pos++;
                	
                	FKLOG("jmp %d", pos);

                    m_cur_stack->m_pos = pos;
        		}
        		break;
            
            case OPCODE_PLUS_ASSIGN:
                {
                    MATH_ASSIGN_OPER(*m_cur_stack, fb, PLUS);
                }
                break;
            case OPCODE_MINUS_ASSIGN:
                {
                    MATH_ASSIGN_OPER(*m_cur_stack, fb, MINUS);
                }
                break;
            case OPCODE_MULTIPLY_ASSIGN:
                {
                    MATH_ASSIGN_OPER(*m_cur_stack, fb, MULTIPLY);
                }
                break;
        	case OPCODE_DIVIDE_ASSIGN:
                {
                    MATH_ASSIGN_OPER(*m_cur_stack, fb, DIVIDE);
                }
                break;
        	case OPCODE_DIVIDE_MOD_ASSIGN:
                {
                    MATH_ASSIGN_OPER(*m_cur_stack, fb, DIVIDE_MOD);
                }
                break;
            case OPCODE_CALL:
                {
					int calltype = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
					m_cur_stack->m_pos++;

                	const variant * callpos = 0;
                	LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "callpos");
                	GET_VARIANT(*m_cur_stack, fb, callpos, m_cur_stack->m_pos);
                	m_cur_stack->m_pos++;

					int retnum = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
					m_cur_stack->m_retnum = retnum; 
					m_cur_stack->m_pos++;

					for (int i = 0; i < retnum; i++)
					{
						assert(ADDR_TYPE(COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos))) == ADDR_STACK);
						m_cur_stack->m_retvpos[i] = m_cur_stack->m_pos;
						m_cur_stack->m_pos++;
					}
                    
                    int argnum = COMMAND_CODE(GET_CMD(fb, m_cur_stack->m_pos));
                	m_cur_stack->m_pos++;

                	paramstack & ps = *getps(m_fk);
                	ps.clear();
                	for (int i = 0; i < argnum; i++)
                	{
                    	variant * arg = 0;
                    	LOG_VARIANT(*m_cur_stack, fb, m_cur_stack->m_pos, "arg");
                    	GET_VARIANT(*m_cur_stack, fb, arg, m_cur_stack->m_pos);
                    	m_cur_stack->m_pos++;

                        variant * argdest = 0;
                    	PS_PUSH_AND_GET(ps, argdest);
                    	*argdest = *arg;
                	}
                	
                    call(m_fk, *callpos, &ps, calltype);
                }
                break;
            default:
                assert(0);
                FKERR("next err code %d %s", code, OpCodeStr(code));
                break;
            }

            if (err)
            {
                // ��������
                m_isend = true;
            }
            num++;
            if (m_isend)
            {
                break;
            }
        }

        return num;
    }

private:
    void call(fake * fk, const variant & callpos, paramstack * ps, int calltype);
    void beginfuncprofile();
    void endfuncprofile();
    
public:
    fake * m_fk;
    bool m_isend;
	variant m_ret[MAX_FAKE_RETURN_NUM];
	stack * m_cur_stack;
	array<stack> m_stack_list;
	processor * m_processor;
};

#define INTER_DELETE(inter) \
	for (int i = 0; i < (int)ARRAY_MAX_SIZE((inter).m_stack_list); i++)\
	{\
	    STACK_DELETE(ARRAY_GET((inter).m_stack_list, i));\
	}
	
#define INTER_INI(inter, fk) (inter).m_fk = fk;\
    ARRAY_INI((inter).m_stack_list, fk)
    
#define INTER_CLEAR(inter) (inter).m_isend = false;\
    (inter).m_cur_stack = 0;\
    ARRAY_CLEAR((inter).m_stack_list);
    
#define INTER_SET_PRO(inter, pro) (inter).m_processor = pro