import re

def remove_function_bodies(file_path):
    # 读取C语言文件的内容
    with open(file_path, 'r') as file:
        content = file.read()

    # 正则表达式匹配函数的定义和删除函数体
    # 匹配函数定义和函数体的正则表达式
    pattern = r'(\b\w+\s+\w+\s*\([^)]*\)\s*)\{[^}]*\}'

    # 用分号替换函数体
    modified_content = re.sub(pattern, r'\1;', content)

    # 将修改后的内容写回到文件
    with open(file_path, 'w') as file:
        file.write(modified_content)

# 输入C语言文件的路径
c_file_path = 'Complier.c'  # 将 'your_c_file.c' 替换为你要处理的 C 语言文件路径

# 调用函数来删除函数体
remove_function_bodies(c_file_path)

print("函数体已被删除并替换为分号。")