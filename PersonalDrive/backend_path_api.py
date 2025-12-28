@bp.route('/path', methods=['GET'])
@jwt_required()
def get_path():
    username = get_jwt_identity()
    dir_id = request.args.get('dir_id', 0, type=int)
    
    # 从数据库获取完整路径信息
    path = FileService.get_path(dir_id, username)
    
    if path is None:
        return jsonify({'msg': 'Directory not found'}), 404
    
    return jsonify(path)

# 在FileService类中添加get_path方法
def get_path(dir_id, username):
    """
    获取从根目录到指定目录的完整路径
    返回格式：[{"id": 0, "name": "个人云盘"}, {"id": 1, "name": "文档"}, ...]
    """
    path = []
    current_id = dir_id
    
    # 递归向上查询父目录，直到根目录
    while current_id is not None:
        file_record = File.query.filter_by(username=username, id=current_id).first()
        if not file_record:
            return None
        
        # 将当前目录添加到路径开头（因为我们是从下往上查询）
        path.insert(0, {
            "id": file_record.id,
            "name": file_record.name,
            "is_dir": file_record.is_dir
        })
        
        # 如果到达根目录，停止查询
        if file_record.parent_id == 0:
            break
        
        # 继续查询父目录
        current_id = file_record.parent_id
    
    # 添加根目录（如果还没有）
    if not path or path[0]['id'] != 0:
        root_dir = File.query.filter_by(username=username, id=0).first()
        if root_dir:
            path.insert(0, {
                "id": root_dir.id,
                "name": root_dir.name,
                "is_dir": root_dir.is_dir
            })
        else:
            # 如果没有找到根目录记录，创建一个默认的
            path.insert(0, {
                "id": 0,
                "name": "个人云盘",
                "is_dir": True
            })
    
    return path

# 确保FileService类已经导入
# from app.services.file_service import FileService