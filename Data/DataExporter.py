import os
import tkinter as tk
from tkinter import filedialog
import pandas as pd

class ExcelToCsvConverterApp:
    CLIENT_INDEX = 0
    SERVER_INDEX = 1

    def __init__(self, root):
        self.root = root
        self.root.title("Excel to CSV Converter")
        self.firstStruct = [True, True]
        self.cpp_content = ["", ""]
        
        self.enumCpp = f"#pragma once\n\n"
        self.enumCpp += f"template<class T>\n"
        self.enumCpp += f"T StringToEnum(const std::string& str)\n{{\n"
        self.enumCpp += f"    return static_cast<T>(std::stoi(str));\n}}\n\n"

        self.load_paths()

        # 엑셀 파일 경로 입력 섹션
        self.excel_dir_label = tk.Label(root, text="Excel Directory:")
        self.excel_dir_label.grid(row=0, column=0, padx=10, pady=5)
        self.excel_dir_entry = tk.Entry(root, width=80)
        self.excel_dir_entry.grid(row=0, column=1, padx=10, pady=5)
        self.excel_dir_button = tk.Button(root, text="Browse", command=lambda: self.browse_excel_directory(1))        
        self.excel_dir_button.grid(row=0, column=2, padx=5, pady=5)
        self.excel_dir_entry.insert(tk.END, self.excel_dir)

        tk.Label(root, text="").grid(row=1, column=0, pady=10)

        # CSV 파일 경로 입력 섹션        
        self.csv_dirs_label1 = tk.Label(root, text="Client CSV Directories:")
        self.csv_dirs_label1.grid(row=2, column=0, padx=10, pady=5)
        self.csv_dirs_entry1 = tk.Entry(root, width=80)
        self.csv_dirs_entry1.grid(row=2, column=1, padx=10, pady=5)
        self.csv_dirs_button1 = tk.Button(root, text="Browse", command=lambda: self.browse_excel_directory(2))
        self.csv_dirs_button1.grid(row=2, column=2, padx=5, pady=5)
        self.csv_dirs_entry1.insert(tk.END, self.csv_dirs[0])

        self.csv_dirs_label2 = tk.Label(root, text="Server CSV Directories:")
        self.csv_dirs_label2.grid(row=3, column=0, padx=10, pady=5)
        self.csv_dirs_entry2 = tk.Entry(root, width=80)
        self.csv_dirs_entry2.grid(row=3, column=1, padx=10, pady=5)
        self.csv_dirs_button2 = tk.Button(root, text="Browse", command=lambda: self.browse_excel_directory(3))
        self.csv_dirs_button2.grid(row=3, column=2, padx=5, pady=5)
        self.csv_dirs_entry2.insert(tk.END, self.csv_dirs[1])

        tk.Label(root, text="").grid(row=4, column=0, pady=10)

        # Enum, Table
        self.enum_dirs_label1 = tk.Label(root, text="Client Enum Directories:")
        self.enum_dirs_label1.grid(row=5, column=0, padx=10, pady=5)
        self.enum_dirs_entry1 = tk.Entry(root, width=80)
        self.enum_dirs_entry1.grid(row=5, column=1, padx=10, pady=5)
        self.enum_dirs_button1 = tk.Button(root, text="Browse", command=lambda: self.browse_directory(3))
        self.enum_dirs_button1.grid(row=5, column=2, padx=5, pady=5)
        self.enum_dirs_entry1.insert(tk.END, self.enum_dirs[0])

        self.table_datas_label1 = tk.Label(root, text="Client Table Data Directories:")
        self.table_datas_label1.grid(row=6, column=0, padx=10, pady=5)
        self.table_datas_entry1 = tk.Entry(root, width=80)
        self.table_datas_entry1.grid(row=6, column=1, padx=10, pady=5)
        self.table_datas_button1 = tk.Button(root, text="Browse", command=lambda: self.browse_directory(4))
        self.table_datas_button1.grid(row=6, column=2, padx=5, pady=5)
        self.table_datas_entry1.insert(tk.END, self.table_dirs[0])

        self.enum_dirs_label2 = tk.Label(root, text="Server Enum Directories:")
        self.enum_dirs_label2.grid(row=7, column=0, padx=10, pady=5)
        self.enum_dirs_entry2 = tk.Entry(root, width=80)
        self.enum_dirs_entry2.grid(row=7, column=1, padx=10, pady=5)
        self.enum_dirs_button2 = tk.Button(root, text="Browse", command=lambda: self.browse_directory(5))
        self.enum_dirs_button2.grid(row=7, column=2, padx=5, pady=5)
        self.enum_dirs_entry2.insert(tk.END, self.enum_dirs[1])

        self.table_datas_label2 = tk.Label(root, text="Server Table Data Directories:")
        self.table_datas_label2.grid(row=8, column=0, padx=10, pady=5)
        self.table_datas_entry2 = tk.Entry(root, width=80)
        self.table_datas_entry2.grid(row=8, column=1, padx=10, pady=5)
        self.table_datas_button2 = tk.Button(root, text="Browse", command=lambda: self.browse_directory(6))
        self.table_datas_button2.grid(row=8, column=2, padx=5, pady=5)
        self.table_datas_entry2.insert(tk.END, self.table_dirs[1])

        self.convert_button = tk.Button(root, text="Convert", command=self.convert)
        self.convert_button.grid(row=9, column=1, pady=10)   

    def browse_directory(self, index):
        directory = filedialog.askopenfilename()
        if directory:
            if index == 3:
                self.enum_dirs_entry1.delete(0, tk.END)
                self.enum_dirs_entry1.insert(tk.END, directory)
            elif index == 4:
                self.table_datas_entry1.delete(0, tk.END)
                self.table_datas_entry1.insert(tk.END, directory)
            elif index == 5:
                self.enum_dirs_entry2.delete(0, tk.END)
                self.enum_dirs_entry2.insert(tk.END, directory)
            elif index == 6:
                self.table_datas_entry2.delete(0, tk.END)
                self.table_datas_entry2.insert(tk.END, directory)

    def browse_excel_directory(self, index):
        directory = filedialog.askdirectory()
        if directory:
            if index == 1:
                self.excel_dir_entry.delete(0, tk.END)
                self.excel_dir_entry.insert(tk.END, directory)
            elif index == 2:
                self.csv_dirs_entry1.delete(0, tk.END)
                self.csv_dirs_entry1.insert(tk.END, directory)
            elif index == 3:
                self.csv_dirs_entry2.delete(0, tk.END)
                self.csv_dirs_entry2.insert(tk.END, directory)

    def convert(self):
        self.excel_dir = self.excel_dir_entry.get()

        csv_dir1 = self.csv_dirs_entry1.get()
        csv_dir2 = self.csv_dirs_entry2.get()
        self.csv_dirs = [csv_dir1, csv_dir2]

        enum_dir1 = self.enum_dirs_entry1.get()
        enum_dir2 = self.enum_dirs_entry2.get()
        self.enum_dirs = [enum_dir1, enum_dir2]

        table_dir1 = self.table_datas_entry1.get()
        table_dir2 = self.table_datas_entry2.get()
        self.table_dirs = [table_dir1, table_dir2]

        self.save_paths()
        self.excel_to_csv()
    
    def save_paths(self):
        with open("paths.txt", "w") as file:
            file.write(f"Excel Directory: {self.excel_dir}\n")
            file.write(f"CSV Directories: {self.csv_dirs}\n")
            file.write(f"Enum Directories: {self.enum_dirs}\n")
            file.write(f"Table Directories: {self.table_dirs}\n")
    
    def load_paths(self):
        try:
            with open("paths.txt", "r") as file:
                lines = file.readlines()
                self.excel_dir = ":".join(lines[0].split(":")[1:]).strip()
                self.csv_dirs = eval(":".join(lines[1].split(":")[1:]).strip())
                self.enum_dirs = eval(":".join(lines[2].split(":")[1:]).strip())
                self.table_dirs = eval(":".join(lines[3].split(":")[1:]).strip())
        except Exception:
            self.excel_dir = ""
            self.csv_dirs = [" ", " "]
            self.enum_dirs = [" ", " "]
            self.table_dirs = [" ", " "]        

    def writeStructFile(self, index):
        self.cpp_content[index] += f"}} // namespace tabledata\n"

        table_dir = self.table_dirs[index]
        with open(table_dir, 'w') as f:
               f.write(self.cpp_content[index])

    def writeEnumFile(self):
        for enum_dir in self.enum_dirs:
            with open(enum_dir, 'w') as f:
                   f.write(self.enumCpp)

    def excel_to_csv(self):
        for filename in os.listdir(self.excel_dir):
            excel_file = os.path.join(self.excel_dir, filename)
            
            if filename.startswith('~$'):
                continue

            # 파일인지 확인
            if os.path.isfile(excel_file) and filename.endswith('.xlsm'):
                self.excel_to_csv_single(excel_file)

        self.writeStructFile(ExcelToCsvConverterApp.CLIENT_INDEX)
        self.writeStructFile(ExcelToCsvConverterApp.SERVER_INDEX)
        
        self.writeEnumFile()

    def excel_to_csv_single(self, excel_file):
        xls = pd.ExcelFile(excel_file)
        
        df = pd.read_excel(excel_file, sheet_name=xls.sheet_names[0])
        self.headerForStruct = df.iloc[:3]

        # 각 시트를 CSV로 변환
        for sheet_index, sheet_name in enumerate(xls.sheet_names):
            if sheet_name.startswith('enum'):
                self.create_enum_class(excel_file, sheet_name)
                continue

            # "-" 이전의 문자열 추출
            csv_filename = sheet_name.split("-")[0] + ".csv"
            
            # 시트 읽기
            if sheet_index == 0:
                df = pd.read_excel(excel_file, sheet_name=sheet_name)
                header_rows = df.iloc[:2]  # 첫 번째와 두 번째 행 읽기
                df = pd.read_excel(excel_file, sheet_name=sheet_name, skiprows=[1])
            else:
                df = pd.read_excel(excel_file, sheet_name=sheet_name)
                header_rows = df.iloc[:2]
                df = pd.read_excel(excel_file, sheet_name=sheet_name, skiprows=[1, 2])
            
            for column in df.columns:
                if column not in df.columns:
                    continue

                value = header_rows.iloc[1][column]

                # 값이 문자열이 아니면 문자열로 변환
                if not isinstance(value, str):
                    value = str(value)

                if value.startswith("enum"):
                    enum_sheet = value.strip()
            
                    try:
                        enum_sheet_df = pd.read_excel(excel_file, sheet_name=enum_sheet)

                        # 해당 열에서 각 행의 값에 대해 매칭되는 행 번호를 가져와서 리스트에 저장
                        enum_row_indices = []
                        for index, row in df.iterrows():
                            enum_value = row[column]
                            if enum_value == 0:
                                break
                            enum_row_index = enum_sheet_df[enum_sheet_df.eq(enum_value).any(axis=1)].index[0]
                            enum_row_indices.append(enum_row_index)

                        # 각 행의 값이 대체될 행 번호 리스트를 열에 대입함
                        df[column] = enum_row_indices
                    except IndexError:
                        print(f"No matching value found in sheet '{enum_sheet}' for column '{column}'.")
                    except Exception as e:
                        print(f"An error occurred while processing column '{column}': {e}")
                elif pd.isnull(df[column]).any():
                    df[column].fillna("NULL", inplace=True)

            # #Name_Helper 값이 있는 열 제외
            df = df.loc[:, ~df.columns.str.contains('^#Name_Helper$')]        
            
            # 클라 디렉토리
            client_df = df.loc[:, ~df.columns.str.contains('^#Server$')]
            csv_path = os.path.join(self.csv_dirs[ExcelToCsvConverterApp.CLIENT_INDEX], csv_filename)
            mode = 'a' if sheet_index > 0 and 'skill_info' in csv_filename else 'w'
            header = False if sheet_index > 0 and 'skill_info' in csv_filename else True
            client_df.to_csv(csv_path, mode=mode, header=header, index=False)

            # 서버 디렉토리
            server_df = df.loc[:, ~df.columns.str.contains('^#Client$')]
            csv_path = os.path.join(self.csv_dirs[ExcelToCsvConverterApp.SERVER_INDEX], csv_filename)
            mode = 'a' if sheet_index > 0 and 'skill_info' in csv_filename else 'w'
            header = False if sheet_index > 0 and 'skill_info' in csv_filename else True
            server_df.to_csv(csv_path, mode=mode, header=header, index=False)


        self.headerForStruct = self.headerForStruct.loc[:, ~self.headerForStruct.columns.str.contains('^#Name_Helper$')]        
        client_header = self.headerForStruct.loc[:, ~self.headerForStruct.columns.str.contains('^#Server$')]
        server_header = self.headerForStruct.loc[:, ~self.headerForStruct.columns.str.contains('^#Client$')]
        struct_name = xls.sheet_names[0].split("-")[0].title().replace('_', '')
        self.create_struct_from_csv(client_header, self.table_dirs[0], struct_name, True)
        self.create_struct_from_csv(server_header, self.table_dirs[1], struct_name, False)

    def create_enum_class(self, excel_file, sheet_name):
        df = pd.read_excel(excel_file, sheet_name=sheet_name)
                
        for column_index, column in enumerate(df.columns):
            enum_name = column
            enum_type = df.iloc[0][column]
            
            self.enumCpp += f"enum class {enum_name} : {enum_type}\n{{\n"
            self.enumCpp += f"    None = 0,\n"
            enum_data = df[column][1:].tolist()
            for index, enum_member in enumerate(enum_data, start=1):
                if pd.notna(enum_member):
                    self.enumCpp += f"    {enum_member} = {index},\n"
            max_value = index + 1
            self.enumCpp += f"    Max = {max_value}\n}};\n\n"

            self.enumCpp += f"template<>\n"
            self.enumCpp += f"{enum_name} StringToEnum(const std::string& str) {{\n"
            self.enumCpp += f"    static std::unordered_map<std::string, {enum_name}> enumMap = {{\n    "
            enum_data = df[column][1:].tolist()
            for index, enum_member in enumerate(enum_data, start=1):
                if pd.notna(enum_member):
                    self.enumCpp += f"{{" + f"\"{enum_member}\", {enum_name}::{enum_member}" + "}, "
            self.enumCpp += "    };\n"
            self.enumCpp += "    auto it = enumMap.find(str);\n"
            self.enumCpp += "    if (it != enumMap.end()) {\n"
            self.enumCpp += "        return it->second;\n"
            self.enumCpp += "    } else {\n"
            self.enumCpp += "        throw std::invalid_argument(\"Invalid enum string\");\n"
            self.enumCpp += "    }\n}\n\n"

    def create_struct_from_csv(self, csv_file, table_dir, struct_name, is_client):        
        df = csv_file

        index = ExcelToCsvConverterApp.SERVER_INDEX
        if is_client:
            index = ExcelToCsvConverterApp.CLIENT_INDEX

        variable_names = []
        for column in df.columns:    
            if '(' in column:
                column = column[:column.index('(')]
            variable_names.extend([cell.strip('#') for cell in column.split()])

        # 두 번째 행에서 자료형 추출
        data_types = []
        for cell in df.iloc[1]:
            if cell.startswith('enum'):
                data_type = cell[cell.find('(') + 1 : cell.find(')')]
            else:
                data_type = cell
                if data_type == "string":
                    if is_client == True:
                        data_type = "FString"
                    else:
                        data_type = "std::string"
                elif data_type in ["uint8_t", "uint16_t", "uint32_t", "uint64_t"] and is_client == True:
                    data_type = data_type.split("_")[0]
            data_types.append(data_type)

        # C++ 구조체 생성
        if self.firstStruct[index]:
            self.cpp_content[index] =   f'#pragma once\n\nnamespace tabledata {{\n\n'
        self.cpp_content[index] += f'struct {struct_name} {{\n'
        for name, data_type in zip(variable_names, data_types):
            initialization = ""
            if data_type == "std::string":
                initialization = ""
            elif data_type in ["uint8_t", "uint16_t", "uint32_t", "uint64_t"]:
                initialization = " = 0"
            elif data_type in ["float", "double"]:
                initialization = " = 0.f"
            elif data_type.startswith("E"):
                initialization = f" = {data_type}::None"
            self.cpp_content[index] += f'    {data_type} {name}{initialization};\n'

        self.cpp_content[index] += '};\n\n'
        self.firstStruct[index] = False

if __name__ == "__main__":
    root = tk.Tk()
    app = ExcelToCsvConverterApp(root)
    root.mainloop()