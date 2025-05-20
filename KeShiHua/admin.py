from django.contrib import admin
from .models import Users, Info
from openpyxl import Workbook
from django.http import HttpResponse

admin.site.site_title = "基于聚类算法的SUV价格与市场趋势分析"
admin.site.site_header = "基于聚类算法的SUV价格与市场趋势分析"


class ExportExcelMixin(object):
    def export_as_excel(self, request, queryset):
        meta = self.model._meta
        field_names = [field.name for field in meta.fields]

        response = HttpResponse(content_type='application/msexcel')
        response['Content-Disposition'] = f'attachment; filename={meta}.xlsx'
        wb = Workbook()
        ws = wb.active
        ws.append(field_names)
        for obj in queryset:
            for field in field_names:
                data = [f'{getattr(obj, field)}' for field in field_names]
            row = ws.append(data)

        wb.save(response)
        return response

    export_as_excel.short_description = '导出Excel'



@admin.register(Info)
class XinXi_Admin(admin.ModelAdmin):
    list_display = (
    "car_year", "car_name", "car_sale", "car_price", "min_price", "max_price", "car_num", "car_img", "date")
    search_fields = ("car_name",)
    list_filter = ['car_name']


from django.contrib.auth.admin import UserAdmin


@admin.register(Users)
class Users_Admin(UserAdmin, ExportExcelMixin):
    list_display = ("username", "email", "set", "age")
    actions = ['export_as_excel']
    search_fields = ("username",)

    fieldsets = (
        (None, {'fields': ('username', 'password')}),
        ('Personal info', {'fields': ('email', 'age', 'set',)}),
        ('Permissions', {'fields': ('is_active', 'is_staff', 'is_superuser', 'groups', 'user_permissions')}),
        ('Important dates', {'fields': ('last_login', 'date_joined')}),
    )

    add_fieldsets = (
        (None, {
            'classes': ('wide',),
            'fields': ('username', 'email', 'age', 'set', 'password1', 'password2'),
        }),
    )
